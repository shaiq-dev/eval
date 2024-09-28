#include "./eval.h"

int
main(int argc, char *argv[])
{
    char *exp = "1+1*(5*2) / 0";
    printf("%.2f\n", eval(exp));
}