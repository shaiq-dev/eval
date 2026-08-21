## A fast math expression evaluator in C

`eval()` folds each operation as it parses, so
there's no syntax tree to build or free.

```c
#include "eval.h"

double ans;

if (eval("1+2*(3+4)", &ans, NULL) == EVAL_OK)
    printf("%g\n", ans);   // 15
```

### Usage

```c
eval_status eval(const char *expr, double *out, eval_error *err);
```

On success it returns `EVAL_OK` and writes to `*out`. Pass `NULL` for `out` if you only want to
know whether an expression parses, and `NULL` for `err` if you don't need the
position.

```c
double ans;
eval_error e;

eval("1+1*4",        &ans, NULL);   // 5
eval("2^3^2",        &ans, NULL);   // 512, '^' is right associative
eval("-2^2",         &ans, NULL);   // -4, the sign applies after '^'
eval("1+1(5*2)",     &ans, NULL);   // 11, implicit '*' before '('
eval("1/0.0000005",  &ans, NULL);   // 2000000, the divisor test is exact zero

if (eval("1+2/(3 * a)", &ans, &e) != EVAL_OK)
    printf("%s at offset %zu\n", eval_strerror(e.status), e.pos);
    // unsupported character at offset 9
```


### Grammar

```
expr   := term  (('+' | '-') term)*
term   := unary (('*' | '/') unary)*        implicit '*' before '('
unary  := ('+' | '-') unary | power
power  := atom ('^' unary)*                 right associative
atom   := number | '(' expr ')'
number := digit+ ('.' digit*)?
```

`*` and `/` sit at the same precedence and associate left, and so do `+` and
`-`. `^` associates right and binds tighter than a prefix sign. Implicit
multiplication only happens before `(`, so `2(3)` is 6 and `2 3` is an error.

Overflow follows IEEE 754 instead of erroring, so `2^1000000` returns `inf`.

Not supported: exponent notation (`1e5`), hex (`0x10`), a leading decimal
point (`.5`), and the `inf` and `nan` literals. 


### Building

```
make            # the ./eval command line tool
make test       # the suite optimised, then again under ASan and UBSan,
                # then random expressions checked against python
```

### Speed

Apple M4, clang `-O2`, one full parse and evaluate per call:

```
 22.2 ns/eval   3 chars  1+1
 37.5 ns/eval   9 chars  1+2*3-4/5
 43.9 ns/eval  17 chars  (1+2)*(3+4)/(5+6)
 46.9 ns/eval  19 chars  ((((1+2)*3)+4)*5)-6
```

The cost of reading digits is higher than the math, hence integer literals with less than 16 digits avoid calling `strtod` and add up directly from their scanning process. For integers with less than 16 digits, `v * 10 + digit` is accurate as long as it is done in `double`. The fast way, therefore, provides the same result as `strtod`. Fractional or longer literals go through `strtod`, which has an endptr checked against a hand scan of the literal.
