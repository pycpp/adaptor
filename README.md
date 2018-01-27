# Adaptor

**Table Of Contents**

- [Stack Pimpl](#stack-pimpl)

## Stack Pimpl

Fast PIMPL idiom using aligned storage declared on the stack. Avoids any dynamic memory allocation, allowing use of the pimpl idiom without having a complete type during declaration.

```cpp
#include <pycpp/adaptor/stack_pimpl.h>

struct file_impl;
struct file
{
public:
private:
   stack_pimpl<file_impl> impl_;
};
```
