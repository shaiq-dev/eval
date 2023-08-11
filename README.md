## Fast and efficient math expression evalvator in C

It's a simple `eval()` functions that can evaluate mathematical expressions really fast. 

### Usage
```c
#include <eval.h>

int
main(int argc, char *argv[])
{
    double ans;
    
    // Expressions
    ans = eval("1+1*4");    // 5
    ans = eval("10+1*0");   // 10

    // Expressions with Parentheses
    ans = eval("1+1(5*2)");    // 11
    ans = eval("3+(5*2)");     // 13

    // Erros
    ans = eval("2/0");      // Error
}

```