// CMakeProject1.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include <iostream>
#include <mutex>
#include <unordered_set>

#include "mem_manager.h"

// test owner_atomic_shared_ptr, inspired by Nim
template<typename T>
struct OwnerSharedPtrData {
  HeapHead head;
  T value;
};

template<typename T>
class OwnerAtomicSharedPtr {
public:
    using value_type = T;
    using const_value_type = const T;
   OwnerAtomicSharedPtr() {
   }
  OwnerAtomicSharedPtr(OwnerSharedPtrData<T> *data)
      : data_(data) {
  data_->head.owner = reinterpret_cast<int64_t>(this);
    MemObjManager::Instance().AddStackObj(this, data_);
    printf("Init. current owner is %p, ref_count: %lld, this: %p\n",
           data_->head.owner, data_->head.ref_obj.size(), (int64_t)this);
  }

  ~OwnerAtomicSharedPtr() {
    if (IsOwner(data_)) {

      while (true) {
        bool has_changed = false;
        auto heap_addrs = MemObjManager::Instance().GetStackObjToHeap(this);
        printf("heap_addrs size: %lld\n", heap_addrs.size());
        for (auto &&addr : heap_addrs) {
          auto typed_addr = reinterpret_cast<OwnerSharedPtrData<T> *>(addr);
          has_changed = Release(typed_addr);
          if (has_changed) {
            MemObjManager::Instance().RemoveStackObj(this, addr);
            break;
          }
        }
        if (!has_changed) {
          break;
        }
      }
    } else {
      Release(data_);
    }
  }

  OwnerAtomicSharedPtr(OwnerAtomicSharedPtr<T> &rhs) {
    std::lock_guard<std::mutex> guard(rhs.data_->head.data_mut);
    data_->head.ref_obj.insert((int64_t)this);
    data_ = rhs.data_;
  }

  OwnerAtomicSharedPtr(OwnerAtomicSharedPtr<T> &&rhs) {
    std::lock_guard<std::mutex> guard(rhs.data_->head.data_mut);
    data_->head.ref_obj.insert((int64_t)this);
    data_ = rhs.data_;
  }

  OwnerAtomicSharedPtr<T>& operator=(const OwnerAtomicSharedPtr<T>& rhs) {
    std::lock_guard<std::mutex> guard(rhs.data_->head.data_mut);
    rhs.data_->head.ref_obj.insert((int64_t)this);
    this->data_ = rhs.data_;
    printf("operator=. current owner is %p, ref_count: %lld, this: %p\n",
           data_->head.owner, data_->head.ref_obj.size(), (int64_t)this);
    return *this;
  }

  value_type* operator->() { return &(data_->value);
  }
  const_value_type* operator->() const { return &(data_->value); }

private:
  bool Release(OwnerSharedPtrData<T> *heap_addr) {
    printf("call release. current owner is %p, ref_count: %lld, this: %p\n",
           heap_addr->head.owner, heap_addr->head.ref_obj.size(),
           (int64_t)this);
    std::unique_lock<std::mutex> lock(heap_addr->head.data_mut);
    if (IsOwner(heap_addr)) {
      if (heap_addr->head.ref_obj.size() == 0) {
        printf("delete %p\n", heap_addr);
        lock.unlock();
        delete heap_addr;
        // printf("waiting for delete finish\n");
        // lock.lock();
        printf("delete finished.\n");
        return true;
      } else {
        auto next_owner = *(heap_addr->head.ref_obj.begin());
        heap_addr->head.owner =
            reinterpret_cast<HeapHead *>(
                MemObjManager::Instance().GetHeapHead((void *)next_owner))
                ->owner;
        heap_addr->head.ref_obj.erase(next_owner);
        reinterpret_cast<OwnerAtomicSharedPtr<T> *>(next_owner)->data_ =
            nullptr;
        MemObjManager::Instance().RemoveStackObj(this, heap_addr);
        MemObjManager::Instance().AddStackObj((void *)heap_addr->head.owner,
                                              heap_addr);
        printf("change ower. current owner is %p, ref_count: %lld, this: %p\n",
               heap_addr->head.owner, heap_addr->head.ref_obj.size(),
               (int64_t)this);
      }
    } else {
      printf("ref delete %p, addr: %p\n", this, heap_addr);
      heap_addr->head.ref_obj.erase((int64_t)this);
    }
    return false;
  }

private:
  bool IsOwner(OwnerSharedPtrData<T> *heap_addr) {
    return reinterpret_cast<int64_t>(this) == data_->head.owner;
  }

private:
  OwnerSharedPtrData<T> *data_ = nullptr;
};

template<typename T> OwnerAtomicSharedPtr<T> makeOwnerAtomicSharedPtr() {
  void *heap_addr = oa_malloc(sizeof(OwnerSharedPtrData<T>));
  OwnerSharedPtrData<T> *data = new (heap_addr) OwnerSharedPtrData<T>;
  printf("new %p\n", heap_addr);
  return OwnerAtomicSharedPtr<T>(data);
}
