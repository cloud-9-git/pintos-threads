#!/usr/bin/env python3
"""Emit pintos/compile_commands.json from the threads kernel Makefile (make -B -n)."""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path


def main() -> None:
    pintos = Path(__file__).resolve().parent.parent
    build = pintos / "threads" / "build"
    if not (build / "Makefile").is_file():
        print(
            "Missing threads/build/Makefile. Run once: cd pintos/threads && make",
            file=sys.stderr,
        )
        sys.exit(1)

    proc = subprocess.run(
        ["make", "-B", "-n"],
        cwd=build,
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr or proc.stdout or "")
        sys.exit(proc.returncode)

    out: list[dict[str, str]] = []
    pat = re.compile(r"^gcc -c (\S+\.c)\s")
    for line in proc.stdout.splitlines():
        line = line.strip()
        if not line.startswith("gcc -c ") or ".c" not in line:
            continue
        m = pat.match(line)
        if not m:
            continue
        src = (build / m.group(1)).resolve()
        if not src.is_file():
            continue
        out.append({"directory": str(build), "command": line, "file": str(src)})

    out_path = pintos / "compile_commands.json"
    out_path.write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(out)} entries to {out_path}")


if __name__ == "__main__":
    main()
