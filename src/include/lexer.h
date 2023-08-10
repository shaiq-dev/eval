#ifndef LEXER_H
#define LEXER_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        TOKEN_ERROR,
        TOKEN_EOF,
        TOKEN_IDENTIFIER,
        TOKEN_NUMBER,
        TOKEN_PLUS,
        TOKEN_MINUS,
        TOKEN_STAR,
        TOKEN_SLASH,
        TOKEN_CARET,
        TOKEN_LEFT_PAREN,
        TOKEN_RIGHT_PAREN,
        TOKEN_COMMA,
        TOKEN_MAX
    } TokenType;

    typedef struct Token
    {
        TokenType type;
        char *lexeme;
        unsigned int length;
    } Token;

    typedef struct Lexer
    {
        char *start;
        char *cur;
    } Lexer;

    Lexer *lexer_init(char *expression);
    Token lexer_next_token(Lexer *lexer);

#ifdef __cplusplus
}
#endif

#endif /* LEXER_H */