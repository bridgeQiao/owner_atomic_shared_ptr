// CMakeProject1.cpp: 定义应用程序的入口点。
//

#include "owner_atomic_shared_ptr.h"

using namespace std;

struct CircleRefObj {
  OwnerAtomicSharedPtr<CircleRefObj> data;
};

int main() {
  auto a = makeOwnerAtomicSharedPtr<CircleRefObj>();
  auto b = makeOwnerAtomicSharedPtr<CircleRefObj>();
  auto c = makeOwnerAtomicSharedPtr<CircleRefObj>();
  a->data = b;
  b->data = c;
  c->data = a;
  printf("a->data: %p\n", &(a->data));
}
