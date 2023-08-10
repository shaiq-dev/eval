#ifndef EVAL_H
#define EVAL_H

#include "parser.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct EvalOptions
    {
        bool log;
    } EvalOptions;

    static inline double
    eval_expression(ExpressionNode *expr)
    {
        switch (expr->type)
        {
        case NODE_NUMBER:
            return expr->number;
        case NODE_POSITIVE:
            return +eval_expression(expr->unary.operand);
        case NODE_NEGATIVE:
            return -eval_expression(expr->unary.operand);
        case NODE_ADD:
            return eval_expression(expr->binary.left) +
                   eval_expression(expr->binary.right);
        case NODE_SUB:
            return eval_expression(expr->binary.left) -
                   eval_expression(expr->binary.right);
        case NODE_MUL:
            return eval_expression(expr->binary.left) *
                   eval_expression(expr->binary.right);
        case NODE_DIV:
            return eval_expression(expr->binary.left) /
                   eval_expression(expr->binary.right);
        case NODE_POW:
            return pow(eval_expression(expr->binary.left),
                       eval_expression(expr->binary.right));
        }

        return 0;
    }

    static inline double
    eval_(char *expr, EvalOptions options[static 1])
    {
        clock_t start, end;

        if (options->log)
        {
            start = clock();
            printf("[eval()] evalvation for %s\n", expr);
        }

        Parser *parser            = parser_init(expr);
        ExpressionNode *expr_tree = parser_parse_expression(parser, PREC_MIN);
        double ans                = eval_expression(expr_tree);

        if (options->log)
        {
            end = clock();
            printf("[eval()] ans=%f , time=%f\n", ans,
                   (start - end) / CLOCKS_PER_SEC);
        }

        return ans;
    }

#define eval(expr, ...) eval_((exp), &(EvalOptions){.log = false, __VA_ARGS__})

#ifdef __cplusplus
}
#endif

#endif /* EVAL_H */