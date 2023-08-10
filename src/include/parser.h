#ifndef PARSER_H
#define PARSER_H

#include "./lexer.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        NODE_ERROR,
        NODE_NUMBER,
        NODE_POSITIVE,
        NODE_NEGATIVE,
        NODE_ADD,
        NODE_SUB,
        NODE_MUL,
        NODE_DIV,
        NODE_POW
    } NodeType;

    typedef enum
    {
        PREC_MIN,
        PREC_TERM,
        PREC_MUL,
        PREC_DIV,
        PREC_POW,
        PREC_MAX,
    } Precedence;

    typedef struct ExpressionNode ExpressionNode;
    struct ExpressionNode
    {
        NodeType type;

        union
        {
            double number;
            struct
            {
                ExpressionNode *operand;
            } unary;
            struct
            {
                ExpressionNode *left;
                ExpressionNode *right;
            } binary;
        };
    };

    typedef struct Parser
    {
        // [TODO] - Memory Management
        Token cur;
        Token next;
        Lexer *lexer;
    } Parser;

    Parser *parser_init(const char *expression);
    ExpressionNode *parser_parse_expression(Parser *parser,
                                            Precedence curr_operator_prec);
    void parser_free(Parser *parser);

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */