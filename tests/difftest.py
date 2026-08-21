#!/usr/bin/env python3
"""Differential test: random expressions, ./eval against python.

    make test-diff
"""
import random
import re
import subprocess
import sys

MAX_DEPTH = 4


def gen(rng, depth=0):
    r = rng.random()
    if depth >= MAX_DEPTH or r < 0.35:
        if rng.random() < 0.3:
            return f"{rng.randint(0, 1000)}.{rng.randint(0, 999)}"
        return str(rng.randint(0, 1000))
    if r < 0.45:
        return f"({gen(rng, depth + 1)})"
    if r < 0.55:
        return f"{rng.choice('+-')}{gen(rng, depth + 1)}"
    if r < 0.62:
        # small exponents only, to keep the result finite
        return f"{gen(rng, depth + 1)}^{rng.randint(0, 4)}"
    op = rng.choice("+-*/")
    return f"{gen(rng, depth + 1)}{op}{gen(rng, depth + 1)}"


# python ints are arbitrary precision, C has doubles, so force float literals
AS_FLOAT = re.compile(r"(?<![\d.])(\d+)(?![\d.])")


def reference(expr):
    """Returns a float, or None if python and eval should both refuse."""
    py = AS_FLOAT.sub(r"\1.0", expr.replace("^", "**"))
    try:
        v = eval(py, {"__builtins__": {}}, {})
    except ZeroDivisionError:
        return None
    except Exception:
        return None
    if isinstance(v, complex) or v != v or v in (float("inf"), float("-inf")):
        return None
    return float(v)


def run(exprs):
    """Feed expressions to ./eval, one per line, return its stdout lines.

    Errors go to stderr, so a short result means ./eval rejected something.
    """
    proc = subprocess.run(
        ["./eval"], input="\n".join(exprs) + "\n", capture_output=True, text=True
    )
    return proc.stdout.splitlines()


def main():
    rng = random.Random(int(sys.argv[1]) if len(sys.argv) > 1 else 20260819)
    rounds = int(sys.argv[2]) if len(sys.argv) > 2 else 20000

    # keep only what python will evaluate, so answers and output lines pair up
    wanted = []
    skipped = 0
    for _ in range(rounds):
        expr = gen(rng)
        want = reference(expr)
        if want is None:
            skipped += 1
        else:
            wanted.append((expr, want))

    got = run([e for e, _ in wanted])

    if len(got) != len(wanted):
        # ./eval refused something, the batch output is misaligned now, so
        # find it one at a time
        print(f"./eval produced {len(got)} results for {len(wanted)} inputs")
        for expr, want in wanted:
            if len(run([expr])) != 1:
                print(f"MISMATCH {expr}: ./eval refused it, python says {want!r}")
        return 1

    mismatches = 0
    for (expr, want), line in zip(wanted, got):
        value = float(line)
        scale = max(abs(want), abs(value), 1e-300)
        if abs(value - want) / scale > 1e-9:
            print(f"MISMATCH {expr}: eval {value!r} vs python {want!r}")
            mismatches += 1

    print(
        f"{len(wanted)} random expressions compared against python, "
        f"{mismatches} mismatches, {skipped} skipped "
        f"(zero division or non-real result)"
    )
    return 1 if mismatches else 0


if __name__ == "__main__":
    sys.exit(main())
