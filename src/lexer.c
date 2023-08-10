#include "include/lexer.h"
#include <stdbool.h>
#include <stdlib.h>

static inline bool
is_digit(char c)
{
    return c >= '0' && c <= '9';
}
static inline bool
is_whitespace(char c)
{
    return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}
static inline bool
is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

#define BOUND() *lexer->cur == '\0'
#define PEEK() *lexer->cur
#define ADVANCE() (lexer->cur++, lexer->cur[-1])

static inline void
skip_whitespace(Lexer *lexer)
{
    while (true)
    {
        char c = PEEK();
        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            ADVANCE();
            break;
        default:
            return;
        }
    }
}

static inline Token
make_token(Lexer *lexer, TokenType type)
{
    return (Token){.type   = type,
                   .lexeme = (char *)lexer->start,
                   .length = (unsigned int)(lexer->cur - lexer->start)};
}

Token
token_number(Lexer *lexer)
{
    while (is_digit(PEEK())) ADVANCE();

    if (PEEK() == '.')
    {
        ADVANCE();
        while (is_digit(PEEK())) ADVANCE();
    }
    return make_token(lexer, TOKEN_NUMBER);
}

Token
token_identifier(Lexer *lexer)
{
    while (is_alpha(PEEK()) || is_digit(PEEK())) ADVANCE();
    return make_token(lexer, TOKEN_IDENTIFIER);
}

Lexer *
lexer_init(char *expression)
{
    Lexer *l = malloc(1 * sizeof(struct Lexer));
    l->start = expression;
    l->cur   = expression;

    return l;
}

Token
lexer_next_token(Lexer *lexer)
{
    skip_whitespace(lexer);
    lexer->start = lexer->cur;

    if (BOUND())
    {
        return make_token(lexer, TOKEN_EOF);
    }

    char c = ADVANCE();

    if (is_digit(c))
    {
        return token_number(lexer);
    }

    if (is_alpha(c))
    {
        return token_identifier(lexer);
    }

    switch (c)
    {
    case '(':
        return make_token(lexer, TOKEN_LEFT_PAREN);
    case ')':
        return make_token(lexer, TOKEN_RIGHT_PAREN);
    case '+':
        return make_token(lexer, TOKEN_PLUS);
    case '-':
        return make_token(lexer, TOKEN_MINUS);
    case '*':
        return make_token(lexer, TOKEN_STAR);
    case '/':
        return make_token(lexer, TOKEN_SLASH);
    case '^':
        return make_token(lexer, TOKEN_CARET);
    }

    return make_token(lexer, TOKEN_ERROR);
}