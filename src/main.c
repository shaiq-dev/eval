#include "./eval.h"

int
main(int argc, char *argv[])
{
    // char *exp = "1+1*(5*2)";
    char *exp = "2/00";
    printf("%f\n", eval(exp));
}