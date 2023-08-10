## Fast and efficient math expression evalvator in C

It's a simple `eval()` functions that can evaluate mathematical expressions really fast. 

### Usgae
```c
#include <include/eval.h>

int
main(int argc, char *argv[])
{
    double ans;
    
    ans = eval("1+1*4");    // 5
    ans = eval("2/0");      // Error
    ans = eval("10+1*0");   // 10
}

```

### Supported Options
Using modern C, the eval function can be overloaded with a bunch of options to change the behaviour or result. <br />
All these options have a default value

```c
eval("1+1", .option_1=value_1, .option_2=value_2 ...)
```

```c
.log    [bool]  [false]
```