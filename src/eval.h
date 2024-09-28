#ifndef EVAL_H
#define EVAL_H

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * If `new_size` is zero or less, the memory pointed to by `ptr` is freed.
 * Otherwise, `realloc` is used to resize the memory block.
 */
static inline void *
__mem(void *pt, size_t size, size_t new_size)
{
    if (new_size <= 0)
    {
        free(pt);
        return (void *)0;
    }

    void *m = realloc(pt, new_size);

    if (m == (void *)0) exit(1);

    return m;
}

#define mem_alloc(T, c) (T *)__mem((void *)0, 0, c * sizeof(T))
#define mem_free(T, pt) (T *)__mem((pt), sizeof(T), 0)

#define die(...)                                                               \
    {                                                                          \
        fprintf(stderr, __VA_ARGS__);                                          \
        exit(1);                                                               \
    }

/**
 * Token types for lexical analysis
 */
typedef enum
{
    TK_ERROR,       // Invalid or unrecognized token
    TK_EOF,         // End of input
    TK_NUMBER,      // Numeric literal
    TK_PLUS,        // '+' operator
    TK_MINUS,       // '-' operator
    TK_STAR,        // '*' operator
    TK_SLASH,       // '/' operator
    TK_CARET,       // '^' operator (exponentiation)
    TK_LEFT_PAREN,  //'(' left parenthesis
    TK_RIGHT_PAREN, // ')' right parenthesis
} TokenType;

typedef struct __token
{
    TokenType type;
    char *value;
    unsigned int len;
} Token;

typedef struct __lexer
{
    char *start;
    char *cur;
    unsigned int pos;
} Lexer;

#define lex_peek() *lex->cur                  // Peek the current character
#define lex_advn() (lex->cur++, lex->cur[-1]) // Advance to the next character
#define lex_peek() *lex->cur                  // Check for whitespace

#define _is_digit(c) (c >= '0' && c <= '9')
#define _is_space(c) (c == ' ' || c == '\r' || c == '\n' || c == '\t')

static inline Token
lex_make_token(Lexer *lex, TokenType type)
{
    if (type == TK_NUMBER)
    {
        while (_is_digit(lex_peek()))
        {
            lex_advn();
        }

        // Handle floating point numbers
        if (lex_peek() == '.')
        {
            lex_advn();

            while (_is_digit(lex_peek()))
            {
                lex_advn();
            }
        }
    }

    // Update position
    unsigned int len = (unsigned int)(lex->cur - lex->start);
    lex->pos += len;

    return (Token){
        .type  = type,
        .value = (char *)lex->start,
        .len   = len,
    };
}

static inline Token
lex_next_token(Lexer *lex)
{
    // Skip whitespaces
    while (_is_space(lex_peek()))
    {
        lex_advn();
    }

    lex->start = lex->cur;

    if (*lex->cur == '\0')
    {
        return lex_make_token(lex, TK_EOF);
    }

    char c = lex_advn();

    if (_is_digit(c))
    {
        return lex_make_token(lex, TK_NUMBER);
    }

    switch (c)
    {
    case '(':
        return lex_make_token(lex, TK_LEFT_PAREN);
    case ')':
        return lex_make_token(lex, TK_RIGHT_PAREN);
    case '+':
        return lex_make_token(lex, TK_PLUS);
    case '-':
        return lex_make_token(lex, TK_MINUS);
    case '*':
        return lex_make_token(lex, TK_STAR);
    case '/':
        return lex_make_token(lex, TK_SLASH);
    case '^':
        return lex_make_token(lex, TK_CARET);
    }

    // Unrecognized character
    return lex_make_token(lex, TK_ERROR);
}

typedef enum
{
    ND_ERROR,
    ND_NUMBER,
    ND_POSITIVE,
    ND_NEGATIVE,
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_POW
} NodeType;

/**
 * Precedence levels for different operators.
 */
typedef enum
{
    PREC_MIN,  // Lowest precedence (used for initial parsing)
    PREC_TERM, // Precedence for '+' and '-'
    PREC_MUL,  // Precedence for '*'
    PREC_DIV,  // Precedence for '/'
    PREC_POW,  // Precedence for '^'
} Precedence;

// Operator precedence table
static Precedence precedence[] = {
    [TK_PLUS] = PREC_TERM, [TK_MINUS] = PREC_TERM, [TK_STAR] = PREC_MUL,
    [TK_SLASH] = PREC_DIV, [TK_CARET] = PREC_POW,
};

/**
 * Represents a node in the abstract syntax tree (AST). Each node can be a
 * number, a unary operation (positive/negative), or a binary operation
 * (addition, subtraction, etc.).
 */
typedef struct __expression_node
{
    NodeType type;

    union
    {
        double number;

        struct
        {
            struct __expression_node *operand;
        } unary;

        struct
        {
            struct __expression_node *left;
            struct __expression_node *right;
        } binary;
    };
} ExpressionNode;

typedef struct __parser
{
    Token cur;
    Lexer *lex;
    char *expr;
} Parser;

static inline ExpressionNode *parser_parse_expr(Parser *p,
                                                Precedence curr_operator_prec);

static inline void
parser_check_error(Parser *p)
{
    if (p->cur.type == TK_ERROR)
        die("\t\"%s\"\n\t%*c\neval: unsupported operand\n", p->expr,
            p->lex->pos + 1, '^');
}

#define parser_advn() (p->cur = lex_next_token(p->lex), parser_check_error(p))

static inline ExpressionNode *
parser_parse_number(Parser *p)
{
    double value = strtod(p->cur.value, NULL);
    parser_advn();

    ExpressionNode *ret = mem_alloc(struct __expression_node, 1);
    ret->type           = ND_NUMBER;
    ret->number         = value;

    return ret;
}

/**
 * Parses prefix expressions (numbers, parentheses, unary operators).
 * Handles numbers, expressions within parentheses, and unary plus/minus.
 * Also handles implicit multiplication (e.g., `2(3)` as `2 * 3`).
 */
static inline ExpressionNode *
parser_parse_prefix_expr(Parser *p)
{
    ExpressionNode *ret = NULL;

    switch (p->cur.type)
    {
    case TK_NUMBER:
        ret = parser_parse_number(p);
        break;

    case TK_LEFT_PAREN:
    {
        parser_advn();
        ret = parser_parse_expr(p, PREC_MIN);
        if (p->cur.type == TK_RIGHT_PAREN) parser_advn();
    }
    break;

    case TK_PLUS:
    {
        parser_advn();
        ret                = mem_alloc(ExpressionNode, 1);
        ret->type          = ND_POSITIVE;
        ret->unary.operand = parser_parse_prefix_expr(p);
    }
    break;

    case TK_MINUS:
    {
        parser_advn();
        ret                = mem_alloc(ExpressionNode, 1);
        ret->type          = ND_NEGATIVE;
        ret->unary.operand = parser_parse_prefix_expr(p);
    }
    break;
    }

    if (!ret)
    {
        ret       = mem_alloc(ExpressionNode, 1);
        ret->type = ND_ERROR;
    }

    // Handle implicit multiplication (e.g., `2(3)` or `(2)(3)`)
    if (p->cur.type == TK_NUMBER || p->cur.type == TK_LEFT_PAREN)
    {
        ExpressionNode *new_ret = mem_alloc(ExpressionNode, 1);
        new_ret->type           = ND_MUL;
        new_ret->binary.left    = ret;
        new_ret->binary.right   = parser_parse_expr(p, PREC_DIV);
        ret                     = new_ret;
    }

    return ret;
}

/**
 * Parses infix expressions based on the operator.
 * Creates a binary operation node and recursively parses the right-hand side
 * expression with the appropriate precedence.
 */
static inline ExpressionNode *
parser_parse_infix_expr(Parser *p, Token op, ExpressionNode *left)
{
    ExpressionNode *ret = mem_alloc(ExpressionNode, 1);

    switch (op.type)
    {
    case TK_PLUS:
        ret->type = ND_ADD;
        break;
    case TK_MINUS:
        ret->type = ND_SUB;
        break;
    case TK_STAR:
        ret->type = ND_MUL;
        break;
    case TK_SLASH:
        ret->type = ND_DIV;
        break;
    case TK_CARET:
        ret->type = ND_POW;
        break;
    }

    ret->binary.left  = left;
    ret->binary.right = parser_parse_expr(p, precedence[op.type]);
    return ret;
}

/**
 * Parses an expression based on operator precedence.
 * Implements a recursive descent parser with operator precedence handling.
 */
static inline ExpressionNode *
parser_parse_expr(Parser *p, Precedence curr_operator_prec)
{
    ExpressionNode *left          = parser_parse_prefix_expr(p);
    Token next_operator           = p->cur;
    Precedence next_operator_prec = precedence[p->cur.type];

    // Continue parsing while the next operator has higher precedence
    while (next_operator_prec != PREC_MIN)
    {

        if (curr_operator_prec >= next_operator_prec) break;

        parser_advn();
        left               = parser_parse_infix_expr(p, next_operator, left);
        next_operator      = p->cur;
        next_operator_prec = precedence[p->cur.type];
    }

    return left;
}

static inline double
__eval(ExpressionNode *expr)
{
    switch (expr->type)
    {
    case ND_NUMBER:
        return expr->number;
    case ND_POSITIVE:
        return +__eval(expr->unary.operand);
    case ND_NEGATIVE:
        return -__eval(expr->unary.operand);
    case ND_ADD:
        return __eval(expr->binary.left) + __eval(expr->binary.right);
    case ND_SUB:
        return __eval(expr->binary.left) - __eval(expr->binary.right);
    case ND_MUL:
        return __eval(expr->binary.left) * __eval(expr->binary.right);
    case ND_DIV:
    {
        double nr = __eval(expr->binary.left);
        double dr = __eval(expr->binary.right);

        if (fabs(dr) < 10e-7) die("eval: division by zero\n");

        return nr / dr;
    }

    case ND_POW:
        return pow(__eval(expr->binary.left), __eval(expr->binary.right));
    }

    return 0;
}

double
eval(char *expr)
{

    Lexer *l = mem_alloc(Lexer, 1);
    l->start = expr;
    l->cur   = expr;
    l->pos   = 0;

    Parser *p = mem_alloc(Parser, 1);
    p->expr   = expr;
    p->cur    = (Token){0};
    p->lex    = l;

    // Get the first token
    parser_advn();

    ExpressionNode *expr_tree = parser_parse_expr(p, PREC_MIN);
    double ans                = __eval(expr_tree);

    // Free Memory
    mem_free(Lexer, l);
    mem_free(Parser, p);

    return ans;
}

#endif