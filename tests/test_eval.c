/* test suite for eval.h
    
    make test 
*/
#include "eval.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int checks = 0;
static int failures = 0;

static void fail(const char *expr, const char *fmt, ...)
{
  va_list ap;
  failures++;
  fprintf(stderr, "FAIL  %-24s ", expr);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
}

typedef struct {
  const char *expr;
  double want;
} value_case;

static const value_case values[] = {
  /* literals and whitespace */
  {"0", 0},
  {"1", 1},
  {"42", 42},
  {"1.5", 1.5},
  {"1.", 1},
  {"0.25", 0.25},
  {"100.125", 100.125},
  {"  7  ", 7},
  {"\t 1 \n+\r 2 \v", 3},
  {"1000000", 1000000},
  {"007", 7},
  {"0.0", 0},

  /* the 15 digit exact path, and past it */
  {"999999999999999", 999999999999999.0},
  {"999999999999999+1", 1000000000000000.0},
  {"1000000000000000", 1000000000000000.0},
  {"12345678901234567890", 12345678901234567890.0},
  {"123456789012345.5", 123456789012345.5},
  {"999999999999999.0", 999999999999999.0},

  /* additive, left associative */
  {"1+1", 2},
  {"1-1", 0},
  {"2-3-4", -5},
  {"8-4+2", 6},
  {"1+2+3+4+5", 15},
  {"10-1-2-3", 4},
  {"1.5+2.25", 3.75},

  /* '*' and '/' share a level, left associative */
  {"2*3", 6},
  {"7/2", 3.5},
  {"6/3*2", 4},
  {"2*6/3", 4},
  {"1/2*4", 2},
  {"100/5*2", 40},
  {"8/4/2", 1},
  {"8/2/2", 2},
  {"100/10/5", 2},
  {"2*3*4", 24},

  /* additive against multiplicative */
  {"1+1*4", 5},
  {"10+1*0", 10},
  {"2+3*4", 14},
  {"2*3+4", 10},
  {"1+2-3*4/6", 1},
  {"1+2*3-4/2", 5},

  /* parentheses */
  {"(1)", 1},
  {"(1+2)", 3},
  {"3+(5*2)", 13},
  {"(1+2)*3", 9},
  {"2*(3+4)", 14},
  {"((((5))))", 5},
  {"(2+3)*(4+5)", 45},
  {"((1+2)*(3+4))/7", 3},
  {"(1+(2*(3+(4*5))))", 47},

  /* implicit multiplication before '(' */
  {"2(3)", 6},
  {"1+1(5*2)", 11},
  {"(2)(3)", 6},
  {"2(3)(4)", 24},
  {"2(3+4)", 14},
  {"(1+2)(3+4)", 21},
  {"-2(3)", -6},
  {"2(3)^2", 18},
  {"2(3)*4", 24},
  {"2*3(4)", 24},

  /* prefix signs */
  {"-1", -1},
  {"+1", 1},
  {"--1", 1},
  {"---1", -1},
  {"+-+-1", 1},
  {"-1+2", 1},
  {"2*-3", -6},
  {"2/-4", -0.5},
  {"3--2", 5},
  {"3-+2", 1},
  {"-(1+2)", -3},
  {"-(-(-3))", -3},

  /* '^', right associative, binds tighter than a prefix sign */
  {"2^3", 8},
  {"2^0", 1},
  {"0^0", 1},
  {"2^10", 1024},
  {"4^0.5", 2},
  {"9^0.5", 3},
  {"2^-1", 0.5},
  {"2^3^2", 512},
  {"2^2^3", 256},
  {"(2^3)^2", 64},
  {"-2^2", -4},
  {"-2^-2", -0.25},
  {"-2^3", -8},
  {"2^3*2", 16},
  {"2*2^3", 16},
  {"2^3+1", 9},
  {"1+2*3^2", 19},
  {"2*3^2", 18},
  {"(1+2)*3^2", 27},
  {"1-2^3/4", -1},
  {"2^2*3", 12},
  {"1^1000000", 1},
  {"-4^0.5*1", -2},
  {"-4^0.5", -2},

  /* small divisors are not zero */
  {"1/0.0000005", 2000000},
  {"1/0.0000001", 10000000},
  {"0.0000001/1", 0.0000001},
  {"0.000001*0.000001", 0.000000000001},
};

typedef struct {
  const char *expr;
  eval_status want;
  size_t pos;
} error_case;

static const error_case errors[] = {
  /* nothing to evaluate */
  {"", EVAL_ERR_UNEXPECTED_END, 0},
  {"   ", EVAL_ERR_UNEXPECTED_END, 3},
  {"1+", EVAL_ERR_UNEXPECTED_END, 2},
  {"1+2*", EVAL_ERR_UNEXPECTED_END, 4},
  {"1++", EVAL_ERR_UNEXPECTED_END, 3},
  {"-", EVAL_ERR_UNEXPECTED_END, 1},
  {"(", EVAL_ERR_UNEXPECTED_END, 1},
  {"2^", EVAL_ERR_UNEXPECTED_END, 2},

  /* operator where a value belongs */
  {"*5", EVAL_ERR_UNEXPECTED_TOKEN, 0},
  {"/5", EVAL_ERR_UNEXPECTED_TOKEN, 0},
  {"^2", EVAL_ERR_UNEXPECTED_TOKEN, 0},
  {")", EVAL_ERR_UNEXPECTED_TOKEN, 0},
  {"1**2", EVAL_ERR_UNEXPECTED_TOKEN, 2},
  {"1+*2", EVAL_ERR_UNEXPECTED_TOKEN, 2},
  {"()", EVAL_ERR_UNEXPECTED_TOKEN, 1},
  {"(*)", EVAL_ERR_UNEXPECTED_TOKEN, 1},

  /* unbalanced parentheses */
  {"(1", EVAL_ERR_UNCLOSED_PAREN, 2},
  {"(1+2", EVAL_ERR_UNCLOSED_PAREN, 4},
  {"((1+2)", EVAL_ERR_UNCLOSED_PAREN, 6},
  {"(1+2(3)", EVAL_ERR_UNCLOSED_PAREN, 7},

  /* a complete expression followed by garbage */
  {"1)", EVAL_ERR_TRAILING_INPUT, 1},
  {"1+2)))", EVAL_ERR_TRAILING_INPUT, 3},
  {"(1+2))", EVAL_ERR_TRAILING_INPUT, 5},
  {"2 3", EVAL_ERR_TRAILING_INPUT, 2},
  {"1 2 3", EVAL_ERR_TRAILING_INPUT, 2},
  {"(3)2", EVAL_ERR_TRAILING_INPUT, 3},

  /* characters outside the grammar */
  {"a", EVAL_ERR_UNEXPECTED_CHAR, 0},
  {"1$", EVAL_ERR_UNEXPECTED_CHAR, 1},
  {"1+2/(3 * a)", EVAL_ERR_UNEXPECTED_CHAR, 9},
  {"1%2", EVAL_ERR_UNEXPECTED_CHAR, 1},
  {".5", EVAL_ERR_UNEXPECTED_CHAR, 0},
  {"1.2.3", EVAL_ERR_UNEXPECTED_CHAR, 3},
  {"nan", EVAL_ERR_UNEXPECTED_CHAR, 0},
  {"inf", EVAL_ERR_UNEXPECTED_CHAR, 0},

  /* forms strtod would accept but this grammar does not */
  {"1e5", EVAL_ERR_UNEXPECTED_CHAR, 1},
  {"0x10", EVAL_ERR_UNEXPECTED_CHAR, 1},
  {"1E5", EVAL_ERR_UNEXPECTED_CHAR, 1},
  {"1_000", EVAL_ERR_UNEXPECTED_CHAR, 1},

  /* arithmetic */
  {"1/0", EVAL_ERR_DIV_ZERO, 1},
  {"2/0", EVAL_ERR_DIV_ZERO, 1},
  {"0/0", EVAL_ERR_DIV_ZERO, 1},
  {"1/0.0", EVAL_ERR_DIV_ZERO, 1},
  {"1/-0", EVAL_ERR_DIV_ZERO, 1},
  {"1/(2-2)", EVAL_ERR_DIV_ZERO, 1},
  {"1+1*(5*2) / 0", EVAL_ERR_DIV_ZERO, 10},
  {"(-8)^0.5", EVAL_ERR_DOMAIN, 4},
  {"(0-4)^0.5", EVAL_ERR_DOMAIN, 5},
  {"-(4)^0.5+(-1)^0.5", EVAL_ERR_DOMAIN, 13},
};

static void check_values(void)
{
  for(size_t i = 0; i < sizeof values / sizeof values[0]; i++) {
    double got;
    eval_error e;
    checks++;

    if(eval(values[i].expr, &got, &e) != EVAL_OK) {
      fail(values[i].expr, "wanted %.17g, got error: %s at %zu",
           values[i].want, eval_strerror(e.status), e.pos);
      continue;
    }

    double want = values[i].want;
    double diff = fabs(got - want);
    double tol = fabs(want) * 1e-12;

    if(diff > tol)
      fail(values[i].expr, "wanted %.17g, got %.17g", want, got);
  }
}

static void check_errors(void)
{
  for(size_t i = 0; i < sizeof errors / sizeof errors[0]; i++) {
    double got = 12345.0;
    eval_error e = {EVAL_OK, 0};
    eval_status st = eval(errors[i].expr, &got, &e);
    checks++;

    if(st != errors[i].want) {
      fail(errors[i].expr, "wanted %s, got %s", eval_strerror(errors[i].want),
           eval_strerror(st));
      continue;
    }
    if(e.pos != errors[i].pos)
      fail(errors[i].expr, "%s reported at %zu, wanted %zu", eval_strerror(st),
           e.pos, errors[i].pos);
    if(got != 12345.0)
      fail(errors[i].expr, "failed but still wrote to *out");
    if(st != e.status)
      fail(errors[i].expr, "return value and err->status disagree");
  }
}

static void check_generated(void)
{
  static char buf[600007];

  struct {
    int depth;
    eval_status want;
  } nests[] = {
    {1, EVAL_OK},
    {64, EVAL_OK},
    {EVAL_MAX_DEPTH - 1, EVAL_OK},
    {EVAL_MAX_DEPTH, EVAL_ERR_TOO_DEEP},
    {EVAL_MAX_DEPTH + 1, EVAL_ERR_TOO_DEEP},
    {10000, EVAL_ERR_TOO_DEEP},
    {200000, EVAL_ERR_TOO_DEEP},
  };

  for(size_t i = 0; i < sizeof nests / sizeof nests[0]; i++) {
    int n = nests[i].depth;
    double got;
    checks++;

    for(int j = 0; j < n; j++)
      buf[j] = '(';
    buf[n] = '7';
    for(int j = 0; j < n; j++)
      buf[n + 1 + j] = ')';
    buf[2 * n + 1] = '\0';

    eval_status st = eval(buf, &got, NULL);
    if(st != nests[i].want)
      fail("nested parens", "depth %d: wanted %s, got %s", n,
           eval_strerror(nests[i].want), eval_strerror(st));
    else if(st == EVAL_OK && got != 7.0)
      fail("nested parens", "depth %d: wanted 7, got %.17g", n, got);
  }

  /* prefix signs recurse as well */
  checks++;
  for(int j = 0; j < 200000; j++)
    buf[j] = '-';
  buf[200000] = '1';
  buf[200001] = '\0';
  if(eval(buf, NULL, NULL) != EVAL_ERR_TOO_DEEP)
    fail("many prefix signs", "wanted too deep");

  /* flat input does not recurse, so no depth limit applies */
  checks++;
  {
    int n = 0;
    buf[n++] = '1';
    for(int j = 0; j < 50000; j++) {
      buf[n++] = '+';
      buf[n++] = '1';
    }
    buf[n] = '\0';

    double got;
    if(eval(buf, &got, NULL) != EVAL_OK || got != 50001.0)
      fail("long flat sum", "wanted 50001, got %.17g", got);
  }

  /* more digits than a double holds */
  checks++;
  memset(buf, '9', 400);
  buf[400] = '\0';
  if(eval(buf, NULL, NULL) != EVAL_ERR_BAD_NUMBER)
    fail("400 nines", "wanted malformed number");

  /* 15 digits, the widest that stays exact */
  checks++;
  memset(buf, '1', 15);
  buf[15] = '\0';
  {
    double got;
    if(eval(buf, &got, NULL) != EVAL_OK || got != 111111111111111.0)
      fail("15 ones", "wanted 111111111111111, got %.17g", got);
  }
}

static void check_api(void)
{
  double got;
  eval_error e;

  checks++;
  if(eval(NULL, &got, &e) != EVAL_ERR_UNEXPECTED_END)
    fail("NULL expression", "wanted unexpected end");

  checks++;
  if(eval("1+1", NULL, NULL) != EVAL_OK)
    fail("NULL out", "wanted ok");

  checks++;
  if(eval("1+1", &got, NULL) != EVAL_OK || got != 2.0)
    fail("NULL err", "wanted 2, got %.17g", got);

  checks++; /* overflow gives inf, not an error */
  if(eval("2^1000000", &got, NULL) != EVAL_OK || !isinf(got))
    fail("2^1000000", "wanted infinity");

  checks++; /* inf - inf gives nan, still not an error */
  if(eval("2^1000000-2^1000000", &got, NULL) != EVAL_OK || !isnan(got))
    fail("inf-inf", "wanted NaN");

  checks++; /* a second call carries nothing over from the first */
  if(eval("1+2*(3+4)", &got, NULL) != EVAL_OK || got != 15.0)
    fail("repeat call", "wanted 15, got %.17g", got);
}

static void check_garbage(void)
{
  /* mostly grammar characters, so the parser gets past the first byte */
  static const char alphabet[] = "0123456789.+-*/^() \t()))((("
                                 "eexX,_abz\x01\x7f";
  unsigned long seed = 20260819u;

  for(int round = 0; round < 200000; round++) {
    char buf[33];
    size_t n = (size_t)(seed >> 11) % (sizeof buf - 1);

    for(size_t i = 0; i < n; i++) {
      seed = seed * 6364136223846793005u + 1442695040888963407u;
      buf[i] = alphabet[(seed >> 33) % (sizeof alphabet - 1)];
    }
    buf[n] = '\0';

    double out;
    eval_error e;
    eval_status st = eval(buf, &out, &e);

    /* an error position must land inside the input */
    if(st != EVAL_OK && e.pos > n) {
      fail(buf, "error position %zu is past the end (%zu)", e.pos, n);
      return;
    }
    seed = seed * 6364136223846793005u + 1442695040888963407u;
  }
  checks++;
}

int main(void)
{
  check_values();
  check_errors();
  check_generated();
  check_api();
  check_garbage();

  printf("%d checks, %d failures\n", checks, failures);
  return failures != 0;
}
