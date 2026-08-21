#define _POSIX_C_SOURCE 200809L

#include "eval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *c_caret = "";
static const char *c_error = "";
static const char *c_off = "";

static void colour_init(void)
{
  const char *no = getenv("NO_COLOR");

  if(no != NULL && *no != '\0')
    return;
  if(!isatty(STDERR_FILENO))
    return;

  c_caret = "\033[1;31m";
  c_error = "\033[31m";
  c_off = "\033[0m";
}

static void report(const char *expr, eval_error e, bool echo)
{
  if(echo)
    fprintf(stderr, "  %s\n", expr);
  fprintf(stderr, "  %*s%s^%s\n", (int)e.pos, "", c_caret, c_off);
  fprintf(stderr, "%seval: %s%s\n", c_error, eval_strerror(e.status), c_off);
}

static int run(const char *expr, bool echo)
{
  double ans;
  eval_error e;

  if(eval(expr, &ans, &e) != EVAL_OK) {
    report(expr, e, echo);
    return 1;
  }

  printf("%.10g\n", ans);
  return 0;
}

int main(int argc, char *argv[])
{
  int failed = 0;

  colour_init();

  if(argc > 1) {
    for(int i = 1; i < argc; i++)
      failed |= run(argv[i], true);
    return failed;
  }

  /* prompt on stderr so ./eval > file still shows it. no prompt when stdin
    is not a tty */
  bool prompt = isatty(STDIN_FILENO);
  char line[4096];

  for(;;) {
    if(prompt) {
      fputs("> ", stderr);
      fflush(stderr);
    }

    if(fgets(line, sizeof line, stdin) == NULL)
      break;

    size_t n = strcspn(line, "\n");

    if(line[n] != '\n' && !feof(stdin)) {
      fprintf(stderr, "%seval: expression longer than %zu bytes%s\n", c_error,
              sizeof line - 1, c_off);
      failed = 1;
      int c;
      while((c = getchar()) != '\n' && c != EOF)
        continue;
      continue;
    }

    line[n] = '\0';
    if(line[0] != '\0')
      failed |= run(line, !prompt);
  }

  if(prompt)
    fputc('\n', stderr);

  return failed;
}
