#include "include/parser.h"
#include <stdlib.h>
#include <string.h>

#define ADVANCE() (parser->cur = parser->next, lexer_next_token(parser->lexer))
#define NEW_EXPRESSION_NODE() calloc(1, sizeof(struct ExpressionNode))

static Precedence precedence[] = {
    [TOKEN_PLUS] = PREC_TERM, [TOKEN_MINUS] = PREC_TERM,
    [TOKEN_STAR] = PREC_MUL,  [TOKEN_SLASH] = PREC_DIV,
    [TOKEN_CARET] = PREC_POW,
};

static ExpressionNode *
parse_number(Parser *parser)
{
    double value = strtod(parser->cur.lexeme, (void *)0);
    ADVANCE();

    ExpressionNode *ret = NEW_EXPRESSION_NODE();
    ret->type           = NODE_NUMBER;
    ret->number         = value;
    return ret;
}

static ExpressionNode *
parse_prefix_expr(Parser *parser)
{
    ExpressionNode *ret = (void *)0;

    switch (parser->cur.type)
    {
    case TOKEN_NUMBER:
        ret = parse_number(parser);
        break;

    case TOKEN_LEFT_PAREN:
    {
        ADVANCE();
        ret = parser_parse_expression(parser, PREC_MIN);
        if (parser->cur.type == TOKEN_RIGHT_PAREN) ADVANCE();
    }
    break;

    case TOKEN_PLUS:
    {
        ADVANCE();
        ret                = NEW_EXPRESSION_NODE();
        ret->type          = NODE_POSITIVE;
        ret->unary.operand = parse_prefix_expr(parser);
    }
    break;

    case TOKEN_MINUS:
    {
        ADVANCE();
        ret                = NEW_EXPRESSION_NODE();
        ret->type          = NODE_NEGATIVE;
        ret->unary.operand = parse_prefix_expr(parser);
    }
    break;
    }

    if (!ret)
    {
        ret = NEW_EXPRESSION_NODE();
        memset(ret, 0, sizeof(ExpressionNode));
        ret->type = NODE_ERROR;
    }

    if (parser->cur.type == TOKEN_NUMBER ||
        parser->cur.type == TOKEN_LEFT_PAREN)
    {
        ExpressionNode *new_ret = NEW_EXPRESSION_NODE();
        new_ret->type           = NODE_MUL;
        new_ret->binary.left    = ret;
        new_ret->binary.right   = parser_parse_expression(parser, PREC_DIV);
        ret                     = new_ret;
    }

    return ret;
}

static ExpressionNode *
parse_infix_expr(Parser *parser, Token operator, ExpressionNode * left)
{
    ExpressionNode *ret = NEW_EXPRESSION_NODE();
    memset(ret, 0, sizeof(ExpressionNode));

    switch (operator.type)
    {
    case TOKEN_PLUS:
        ret->type = NODE_ADD;
        break;
    case TOKEN_MINUS:
        ret->type = NODE_SUB;
        break;
    case TOKEN_STAR:
        ret->type = NODE_MUL;
        break;
    case TOKEN_SLASH:
        ret->type = NODE_DIV;
        break;
    case TOKEN_CARET:
        ret->type = NODE_POW;
        break;
    }

    ret->binary.left = left;
    ret->binary.right = parser_parse_expression(parser, precedence[operator.type]);
    return ret;
}

Parser *
parser_init(const char *expression)
{
    Parser *parser = malloc(1 * sizeof(struct Parser));

    parser->cur   = (Token){0};
    parser->next  = (Token){0};
    parser->lexer = lexer_init(expression);

    ADVANCE();
    ADVANCE();

    return parser;
}

ExpressionNode *
parser_parse_expression(Parser *parser, Precedence curr_operator_prec)
{
    ExpressionNode *left          = parse_prefix_expr(parser);
    Token next_operator           = parser->cur;
    Precedence next_operator_prec = precedence[parser->cur.type];

    while (next_operator_prec != PREC_MIN)
    {

        if (curr_operator_prec >= next_operator_prec) break;

        ADVANCE();
        left               = parse_infix_expr(parser, next_operator, left);
        next_operator      = parser->cur;
        next_operator_prec = precedence[parser->cur.type];
    }

    return left;
}

void
parser_free(Parser *parser)
{
    // NOT_IMPLEMENTED
}