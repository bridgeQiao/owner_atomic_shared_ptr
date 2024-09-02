# owner_atomic_shared_ptr

`owner_atomic_shared_ptr` is shared_ptr without reference circle problem. Inspired by Nim's ORC, which means `owner & reference count`.

The main idea of my implementation is each heap memory has `a owner` and `a set` for reference count.

```python
# release
if is_owner:
    if reference_count is 0:
        delete heap_addr
    else:
        change the owner to first reference object which has more longer lifetime
        remove the reference object form the set
else:
    remove self form the set
```
