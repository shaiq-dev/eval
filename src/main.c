#include "include/eval.h"

int
main(int argc, char *argv[])
{
    char *exp  = "1+1";
    double ans = eval(exp, .log = true);
}