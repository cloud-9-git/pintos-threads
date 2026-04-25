#!/usr/bin/env bash
set -euo pipefail
# Repo root = parent of pintos/
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PINTOS="$(cd "${SCRIPT_DIR}/.." && pwd)"
python3 "${PINTOS}/utils/gen_compile_commands.py"
# Cursor/VS Code 원격 UI 표시 언어 (한국어 팩이 설치돼 있어야 함)
USER_DATA="${HOME}/.cursor-server/data/User"
mkdir -p "${USER_DATA}"
printf '%s\n' '{' '  "locale": "ko"' '}' > "${USER_DATA}/argv.json"
