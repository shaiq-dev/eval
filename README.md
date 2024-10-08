## Fast and efficient math expression evaluator in C

A simple `eval()` that can parse and evaluate mathematical expressions really fast. 

### Usage
```c
#include <eval.h>

int
main(int argc, char *argv[])
{
    double ans;
    
    // Expressions
    ans = eval("1+1*4");                // 5
    ans = eval("10+1*0");               // 10

    // Expressions with Parentheses
    ans = eval("1+1(5*2)");             // 11
    ans = eval("3+(5*2)");              // 13

    // Erros
    ans = eval("2/0");                  // eval: division by zero
    
    ans = eval("1+2/(3 * a)")
    //  "1+2/(3 * a)"
    //            ^
    // eval: unsupported operand           
}

```
