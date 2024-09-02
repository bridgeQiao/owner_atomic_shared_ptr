#include <set>
#include <mutex>
#include <map>
#include <unordered_map>

struct HeapHead {
  std::mutex data_mut;
  int64_t owner;
  std::set<int64_t> ref_obj;
};

class MemObjManager {
public:
  static MemObjManager &Instance() {
    static MemObjManager stack_obj_manager;
    return stack_obj_manager;
  }

  MemObjManager &AddStackObj(void *stack_obj, void *mem_addr) {
    if (stack_obj_data_.count(stack_obj) > 0) {
      stack_obj_data_[stack_obj].insert(mem_addr);
    } else {
      stack_obj_data_[stack_obj] = {mem_addr};
    }
    return *this;
  }

  void RemoveStackObj(void *stack_obj, void *mem_addr) {
    if (stack_obj_data_.count(stack_obj) > 0) {
      stack_obj_data_[stack_obj].erase(mem_addr);
    }
  }

  void AddHeapObj(void *mem_obj, size_t len) {
    heap_obj_data_.insert({(int64_t)mem_obj + len, len});
  }

  std::set<void*> GetStackObjToHeap(void* stack_obj) {
    if (stack_obj_data_.count(stack_obj) > 0) {
      return stack_obj_data_[stack_obj];
    }
  }

  void *GetHeapHead(void *mem_obj) {
    auto find_obj = heap_obj_data_.lower_bound((int64_t)mem_obj);
    return reinterpret_cast<void *>(find_obj->first - find_obj->second);
  }
  
private:
  MemObjManager() = default;
  MemObjManager(const MemObjManager &rhs) = delete;

private:
  std::unordered_map<void *, std::set<void *>>
      stack_obj_data_; // stack_addr -> [heap_addr_0, heap_addr_1, ...]
  std::map<int64_t, int> heap_obj_data_; // heap_addr -> len
};

void *oa_malloc(size_t len) {
  void *heap_addr = malloc(len);
  MemObjManager::Instance().AddHeapObj(heap_addr, len);
  return heap_addr;
}
