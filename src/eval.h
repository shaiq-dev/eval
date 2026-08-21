/* eval.h - a small, allocation free math expression evaluator.

 Grammar:
    expr   := term  (('+' | '-') term)*
    term   := unary (('*' | '/') unary)*         implicit '*' before '('
    unary  := ('+' | '-') unary | power
    power  := atom ('^' unary)*                  right associative
    atom   := number | '(' expr ')'
    number := digit+ ('.' digit*)?               no exponent, no leading '.'
 */
#ifndef EVAL_H
#define EVAL_H

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h> /* strtod */

/* recursion limit, a precaution against deeply nested input */
#ifndef EVAL_MAX_DEPTH
#define EVAL_MAX_DEPTH 128
#endif

typedef enum {
  EVAL_OK = 0,
  EVAL_ERR_UNEXPECTED_CHAR,  /* stray byte, not part of the grammar */
  EVAL_ERR_UNEXPECTED_TOKEN, /* an operator or ')' where a number was due */
  EVAL_ERR_UNEXPECTED_END,   /* input ended mid expression */
  EVAL_ERR_UNCLOSED_PAREN,
  EVAL_ERR_TRAILING_INPUT, /* complete expression followed by garbage */
  EVAL_ERR_BAD_NUMBER,     /* numeric literal strtod() could not represent */
  EVAL_ERR_DIV_ZERO,
  EVAL_ERR_DOMAIN, /* '^' outside its domain, e.g. (-8)^0.5 */
  EVAL_ERR_TOO_DEEP
} eval_status;

typedef struct {
  eval_status status;
  size_t pos; /* byte offset into the expression */
} eval_error;

static inline const char *eval_strerror(eval_status status)
{
  switch(status) {
  case EVAL_OK:
    return "ok";
  case EVAL_ERR_UNEXPECTED_CHAR:
    return "unsupported character";
  case EVAL_ERR_UNEXPECTED_TOKEN:
    return "expected a value";
  case EVAL_ERR_UNEXPECTED_END:
    return "unexpected end of expression";
  case EVAL_ERR_UNCLOSED_PAREN:
    return "missing closing parenthesis";
  case EVAL_ERR_TRAILING_INPUT:
    return "unexpected trailing input";
  case EVAL_ERR_BAD_NUMBER:
    return "malformed number";
  case EVAL_ERR_DIV_ZERO:
    return "division by zero";
  case EVAL_ERR_DOMAIN:
    return "result is not a real number";
  case EVAL_ERR_TOO_DEEP:
    return "expression nested too deeply";
  }
  return "unknown error";
}

/* Keep EVAL_T_BAD last, the precedence table is sized from it. */
typedef enum {
  EVAL_T_END = 0,
  EVAL_T_NUMBER,
  EVAL_T_PLUS,
  EVAL_T_MINUS,
  EVAL_T_STAR,
  EVAL_T_SLASH,
  EVAL_T_CARET,
  EVAL_T_LPAREN,
  EVAL_T_RPAREN,
  EVAL_T_BAD,
} eval_token;

enum {
  EVAL_PREC_NONE = 0,
  EVAL_PREC_TERM = 1,  /* + - */
  EVAL_PREC_MUL = 2,   /* * / and the implied one in 2(3) */
  EVAL_PREC_UNARY = 3, /* prefix + - */
  EVAL_PREC_POW = 4    /* ^ */
};

/* binary precedence per token. zero means the token is not a binary
   operator, which ends the operator loop. */
static const unsigned char eval_prec[EVAL_T_BAD + 1] = {
  [EVAL_T_PLUS] = EVAL_PREC_TERM, [EVAL_T_MINUS] = EVAL_PREC_TERM,
  [EVAL_T_STAR] = EVAL_PREC_MUL,  [EVAL_T_SLASH] = EVAL_PREC_MUL,
  [EVAL_T_CARET] = EVAL_PREC_POW,
};

typedef struct {
  const char *begin; /* start of the expression, err_pos is relative to it */
  const char *cur;   /* next character to read */
  const char *tok;   /* first character of the current token */
  eval_token type;
  double num; /* set when type is EVAL_T_NUMBER */
  int depth;
  eval_status err; /* first error seen, or EVAL_OK */
  size_t err_pos;
} eval_parser;

static inline bool eval_is_digit(char c)
{
  return c >= '0' && c <= '9';
}

static inline bool eval_is_space(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
         c == '\f';
}

static inline void eval_fail(eval_parser *p, eval_status status,
                             const char *at)
{
  if(p->err == EVAL_OK) {
    p->err = status;
    p->err_pos = (size_t)(at - p->begin);
  }
}

/* 10^15 - 1 is below 2^53, so accumulating v * 10 + digit stays exact for
   up to this many digits */
#define EVAL_EXACT_DIGITS 15

static inline void eval_lex_number(eval_parser *p, const char *s)
{
  const char *d = s;
  double whole = 0.0;

  while(eval_is_digit(*d) && d - s < EVAL_EXACT_DIGITS) {
    whole = whole * 10.0 + (double)(*d - '0');
    d++;
  }

  if(!eval_is_digit(*d) && *d != '.') {
    p->num = whole;
    p->type = EVAL_T_NUMBER;
    p->cur = d;
    return;
  }

  const char *end = s;
  while(eval_is_digit(*end))
    end++;
  if(*end == '.') {
    end++;
    while(eval_is_digit(*end))
      end++;
  }

  char *stop;
  double value = strtod(s, &stop);

  if(stop > end) {
    /* strtod consumed more than the grammar allows so the extra character at
        'end' is the real problem. 'e' in "1e5", 'x' in "0x10". */
    eval_fail(p, EVAL_ERR_UNEXPECTED_CHAR, end);
    p->type = EVAL_T_BAD;
    return;
  }
  if(stop < end || !isfinite(value)) {
    /* Locale mismatch, or a literal too large to represent. */
    eval_fail(p, EVAL_ERR_BAD_NUMBER, s);
    p->type = EVAL_T_BAD;
    return;
  }

  p->num = value;
  p->type = EVAL_T_NUMBER;
  p->cur = end;
}

static inline void eval_next(eval_parser *p)
{
  const char *s = p->cur;
  while(eval_is_space(*s))
    s++;

  p->tok = s;

  if(eval_is_digit(*s)) {
    eval_lex_number(p, s);
    return;
  }

  p->cur = s + 1;

  switch(*s) {
  case '\0':
    p->cur = s; /* never step past the terminator */
    p->type = EVAL_T_END;
    return;
  case '+':
    p->type = EVAL_T_PLUS;
    return;
  case '-':
    p->type = EVAL_T_MINUS;
    return;
  case '*':
    p->type = EVAL_T_STAR;
    return;
  case '/':
    p->type = EVAL_T_SLASH;
    return;
  case '^':
    p->type = EVAL_T_CARET;
    return;
  case '(':
    p->type = EVAL_T_LPAREN;
    return;
  case ')':
    p->type = EVAL_T_RPAREN;
    return;
  }

  eval_fail(p, EVAL_ERR_UNEXPECTED_CHAR, s);
  p->type = EVAL_T_BAD;
}

static inline double eval_parse(eval_parser *p, unsigned min_prec);

static inline double eval_apply(eval_parser *p, eval_token op,
                                const char *op_pos, double a, double b)
{
  switch(op) {
  case EVAL_T_PLUS:
    return a + b;
  case EVAL_T_MINUS:
    return a - b;
  case EVAL_T_STAR:
    return a * b;

  case EVAL_T_SLASH:
    if(b == 0.0) {
      eval_fail(p, EVAL_ERR_DIV_ZERO, op_pos);
      return 0.0;
    }
    return a / b;

  case EVAL_T_CARET: {
    double r = pow(a, b);
    /* pow() only returns NaN from non NaN input on a domain error, like a
       negative base with a fractional exponent. overflow to infinity is
       not an error here */
    if(isnan(r) && !isnan(a) && !isnan(b)) {
      eval_fail(p, EVAL_ERR_DOMAIN, op_pos);
      return 0.0;
    }
    return r;
  }

  case EVAL_T_END:
  case EVAL_T_NUMBER:
  case EVAL_T_LPAREN:
  case EVAL_T_RPAREN:
  case EVAL_T_BAD:
    break; /* not reachable, the caller checked precedence first */
  }
  return 0.0;
}

/* Parses a number, a parenthesized expression or a prefix sign.

 A prefix sign parses its operand at EVAL_PREC_UNARY, which is below '^',
 so -2^2 becomes -(2^2) and not (-2)^2.
 */
static inline double eval_parse_value(eval_parser *p)
{
  if(p->err != EVAL_OK)
    return 0.0;

  if(p->depth >= EVAL_MAX_DEPTH) {
    eval_fail(p, EVAL_ERR_TOO_DEEP, p->tok);
    return 0.0;
  }
  p->depth++;

  double v = 0.0;

  switch(p->type) {
  case EVAL_T_NUMBER:
    v = p->num;
    eval_next(p);
    break;

  case EVAL_T_LPAREN:
    eval_next(p);
    v = eval_parse(p, EVAL_PREC_TERM);
    if(p->err == EVAL_OK) {
      if(p->type == EVAL_T_RPAREN)
        eval_next(p);
      else
        eval_fail(p, EVAL_ERR_UNCLOSED_PAREN, p->tok);
    }
    break;

  case EVAL_T_PLUS:
    eval_next(p);
    v = eval_parse(p, EVAL_PREC_UNARY);
    break;

  case EVAL_T_MINUS:
    eval_next(p);
    v = -eval_parse(p, EVAL_PREC_UNARY);
    break;

  case EVAL_T_END:
    eval_fail(p, EVAL_ERR_UNEXPECTED_END, p->tok);
    break;

  case EVAL_T_BAD:
    break;

  case EVAL_T_STAR:
  case EVAL_T_SLASH:
  case EVAL_T_CARET:
  case EVAL_T_RPAREN:
    eval_fail(p, EVAL_ERR_UNEXPECTED_TOKEN, p->tok);
    break;
  }

  p->depth--;
  return v;
}

/* Parses and folds every operator whose precedence is at least min_prec. */
static inline double eval_parse(eval_parser *p, unsigned min_prec)
{
  double left = eval_parse_value(p);

  while(p->err == EVAL_OK) {
    eval_token op = p->type;
    const char *pos = p->tok;
    bool implicit = (op == EVAL_T_LPAREN); /* 2(3) is 2*(3) */
    unsigned prec = implicit ? EVAL_PREC_MUL : eval_prec[op];

    if(prec < min_prec)
      break;

    /* passing prec rather than prec + 1 lets another '^' bind on the right,
        which is what makes it right associative. the rest are left */
    unsigned next_min = (op == EVAL_T_CARET) ? prec : prec + 1;

    if(!implicit)
      eval_next(p);

    double right = eval_parse(p, next_min);
    if(p->err != EVAL_OK)
      break;

    left = eval_apply(p, implicit ? EVAL_T_STAR : op, pos, left, right);
  }

  return left;
}

/* eval()

 Evaluates 'expr' and stores the result in 'out'. Returns EVAL_OK, or the
 reason it failed.

 Both 'out' and 'err' can be NULL. A NULL 'out' still evaluates, it just
 throws the result away.
 */
static inline eval_status eval(const char *expr, double *out, eval_error *err)
{
  eval_parser p;
  p.begin = expr;
  p.cur = expr;
  p.tok = expr;
  p.type = EVAL_T_END;
  p.num = 0.0;
  p.depth = 0;
  p.err = EVAL_OK;
  p.err_pos = 0;

  if(expr == NULL) {
    p.err = EVAL_ERR_UNEXPECTED_END;
  }
  else {
    eval_next(&p);
    double value = eval_parse(&p, EVAL_PREC_TERM);

    if(p.err == EVAL_OK && p.type != EVAL_T_END)
      eval_fail(&p, EVAL_ERR_TRAILING_INPUT, p.tok);

    if(p.err == EVAL_OK && out != NULL)
      *out = value;
  }

  if(err != NULL) {
    err->status = p.err;
    err->pos = p.err_pos;
  }
  return p.err;
}

#endif /* EVAL_H */
