#ifndef EVAL_H
#define EVAL_H

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Memory
static inline void *
_memory(void *ptr, size_t size, size_t new_size)
{
    if (new_size <= 0)
    {
        free(ptr);
        return (void *)0;
    }
    void *m = realloc(ptr, new_size);
    if (m == (void *)0) exit(1);

    return m;
}

#define MEM_ALLOC(T, count) (T *)_memory((void *)0, 0, count * sizeof(T))
#define MEM_FREE(T, ptr) (T *)_memory((ptr), sizeof(T), 0)

// Debug
#define DEBUG true
#define dbg(...)                                                               \
    if (DEBUG)                                                                 \
    {                                                                          \
        fprintf(stdout, "[DEBUG] ");                                           \
        fprintf(stdout, __VA_ARGS__);                                          \
        fprintf(stdout, "\n");                                                 \
    }

// Lexer
typedef enum
{
    TOKEN_ERROR,
    TOKEN_EOF,
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_CARET,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
} TokenType;

typedef struct Token
{
    TokenType type;
    char *value;
    unsigned int length;
} Token;

typedef struct Lexer
{
    char *start;
    char *current;
} Lexer;

#define LEXER_PEEK() *l->current
#define LEXER_ADVANCE() (l->current++, l->current[-1])
#define IS_WHITESPACE(c) (c == ' ' || c == '\r' || c == '\n' || c == '\t')
#define IS_DIGIT(c) (c >= '0' && c <= '9')

static inline Token
make_token(Lexer *l, TokenType type)
{
    if (type == TOKEN_NUMBER)
    {
        while (IS_DIGIT(LEXER_PEEK())) LEXER_ADVANCE();

        if (LEXER_PEEK() == '.')
        {
            LEXER_ADVANCE();
            while (IS_DIGIT(LEXER_PEEK())) LEXER_ADVANCE();
        }
    }

    return (Token){.type   = type,
                   .value  = (char *)l->start,
                   .length = (unsigned int)(l->current - l->start)};
}

static inline Token
lexer_next_token(Lexer *l)
{
    // Skip whitespaces
    while (IS_WHITESPACE(LEXER_PEEK())) LEXER_ADVANCE();
    l->start = l->current;

    if (*l->current == '\0') return make_token(l, TOKEN_ERROR);

    char c = LEXER_ADVANCE();

    if (IS_DIGIT(c)) return make_token(l, TOKEN_NUMBER);

    switch (c)
    {
    case '(':
        return make_token(l, TOKEN_LEFT_PAREN);
    case ')':
        return make_token(l, TOKEN_RIGHT_PAREN);
    case '+':
        return make_token(l, TOKEN_PLUS);
    case '-':
        return make_token(l, TOKEN_MINUS);
    case '*':
        return make_token(l, TOKEN_STAR);
    case '/':
        return make_token(l, TOKEN_SLASH);
    case '^':
        return make_token(l, TOKEN_CARET);
    }

    return make_token(l, TOKEN_ERROR);
}

// Parser
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
} Precedence;

typedef struct ExpressionNode
{
    NodeType type;

    union
    {
        double number;
        struct
        {
            struct ExpressionNode *operand;
        } unary;
        struct
        {
            struct ExpressionNode *left;
            struct ExpressionNode *right;
        } binary;
    };
} ExpressionNode;

typedef struct Parser
{
    Token current;
    Lexer *lexer;
} Parser;

#define PARSER_ADVANCE() p->current = lexer_next_token(p->lexer)

static Precedence precedence[] = {
    [TOKEN_PLUS] = PREC_TERM, [TOKEN_MINUS] = PREC_TERM,
    [TOKEN_STAR] = PREC_MUL,  [TOKEN_SLASH] = PREC_DIV,
    [TOKEN_CARET] = PREC_POW,
};

static inline ExpressionNode *parse_expr(Parser *p,
                                         Precedence curr_operator_prec);

static inline ExpressionNode *
parse_number(Parser *p)
{
    double value = strtod(p->current.value, (void *)0);
    PARSER_ADVANCE();

    ExpressionNode *ret = MEM_ALLOC(ExpressionNode, 1);
    ret->type           = NODE_NUMBER;
    ret->number         = value;
    return ret;
}

static inline ExpressionNode *
parse_prefix_expr(Parser *p)
{
    ExpressionNode *ret = (void *)0;

    switch (p->current.type)
    {
    case TOKEN_NUMBER:
        ret = parse_number(p);
        break;

    case TOKEN_LEFT_PAREN:
    {
        PARSER_ADVANCE();
        ret = parse_expr(p, PREC_MIN);
        if (p->current.type == TOKEN_RIGHT_PAREN) PARSER_ADVANCE();
    }
    break;

    case TOKEN_PLUS:
    {
        PARSER_ADVANCE();
        ret                = MEM_ALLOC(ExpressionNode, 1);
        ret->type          = NODE_POSITIVE;
        ret->unary.operand = parse_prefix_expr(p);
    }
    break;

    case TOKEN_MINUS:
    {
        PARSER_ADVANCE();
        ret                = MEM_ALLOC(ExpressionNode, 1);
        ret->type          = NODE_NEGATIVE;
        ret->unary.operand = parse_prefix_expr(p);
    }
    break;
    }

    if (!ret)
    {
        ret       = MEM_ALLOC(ExpressionNode, 1);
        ret->type = NODE_ERROR;
    }

    if (p->current.type == TOKEN_NUMBER || p->current.type == TOKEN_LEFT_PAREN)
    {
        ExpressionNode *new_ret = MEM_ALLOC(ExpressionNode, 1);
        new_ret->type           = NODE_MUL;
        new_ret->binary.left    = ret;
        new_ret->binary.right   = parse_expr(p, PREC_DIV);
        ret                     = new_ret;
    }

    return ret;
}

static inline ExpressionNode *
parse_infix_expr(Parser *p, Token operator, ExpressionNode * left)
{
    ExpressionNode *ret = MEM_ALLOC(ExpressionNode, 1);

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

    ret->binary.left  = left;
    ret->binary.right = parse_expr(p, precedence[operator.type]);
    return ret;
}

static inline ExpressionNode *
parse_expr(Parser *p, Precedence curr_operator_prec)
{
    ExpressionNode *left          = parse_prefix_expr(p);
    Token next_operator           = p->current;
    Precedence next_operator_prec = precedence[p->current.type];

    while (next_operator_prec != PREC_MIN)
    {

        if (curr_operator_prec >= next_operator_prec) break;

        PARSER_ADVANCE();
        left               = parse_infix_expr(p, next_operator, left);
        next_operator      = p->current;
        next_operator_prec = precedence[p->current.type];
    }

    return left;
}

// Evaluator
static inline double
solve(ExpressionNode *expr)
{
    switch (expr->type)
    {
    case NODE_NUMBER:
        return expr->number;
    case NODE_POSITIVE:
        return +solve(expr->unary.operand);
    case NODE_NEGATIVE:
        return -solve(expr->unary.operand);
    case NODE_ADD:
        return solve(expr->binary.left) + solve(expr->binary.right);
    case NODE_SUB:
        return solve(expr->binary.left) - solve(expr->binary.right);
    case NODE_MUL:
        return solve(expr->binary.left) * solve(expr->binary.right);
    case NODE_DIV:
        return solve(expr->binary.left) / solve(expr->binary.right);
    case NODE_POW:
        return pow(solve(expr->binary.left), solve(expr->binary.right));
    }

    return 0;
}

double
eval(char *expr)
{

    Lexer *l   = MEM_ALLOC(Lexer, 1);
    l->start   = expr;
    l->current = expr;

    Parser *p  = MEM_ALLOC(Parser, 1);
    p->current = (Token){0};
    p->lexer   = l;

    // Get the first token
    PARSER_ADVANCE();

    ExpressionNode *expr_tree = parse_expr(p, PREC_MIN);
    double ans                = solve(expr_tree);

    return ans;
}

#endif /* EVAL_H */