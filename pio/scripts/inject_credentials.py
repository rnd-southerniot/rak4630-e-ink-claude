"""
PlatformIO pre-build hook: inject LoRaWAN OTAA credentials as build flags.

Reads firmware/.env (gitignored) and emits:
    -DAPP_DEVEUI="..."   (8-byte hex, 16 chars)
    -DAPP_JOINEUI="..."  (8-byte hex, 16 chars; defaults all-zero)
    -DAPP_APPKEY="..."   (16-byte hex, 32 chars)

Keys never enter source control. If firmware/.env is missing or a value is
blank, the corresponding macro is left undefined and lorawan_service.cpp falls
back to its all-zero placeholder (gates 0-5, 8 still build/run; gates 6/7/9
will fail to join until real credentials are provided).

Hex is validated and upper-cased; format errors abort the build early with a
clear message rather than producing a node that silently never joins.
"""

import os
import re

Import("env")  # noqa: F821  (injected by PlatformIO)

PROJECT_DIR = env.subst("$PROJECT_DIR")  # noqa: F821
REPO_ROOT = os.path.abspath(os.path.join(PROJECT_DIR, ".."))
FW_DIR = os.path.join(REPO_ROOT, "firmware")

# Each physical node has its own DevEUI/AppKey. Prefer a per-board file
# firmware/.env.<pioenv> (e.g. .env.rak3312) so both boards keep distinct
# credentials; fall back to the shared firmware/.env.
PIOENV = env.subst("$PIOENV")  # noqa: F821
_per_board = os.path.join(FW_DIR, ".env." + PIOENV)
ENV_PATH = _per_board if os.path.isfile(_per_board) else os.path.join(FW_DIR, ".env")

# key in .env -> (macro name, expected hex length in chars)
FIELDS = {
    "DEVEUI": ("APP_DEVEUI", 16),
    "JOINEUI": ("APP_JOINEUI", 16),
    "APPKEY": ("APP_APPKEY", 32),
}

_HEX_RE = re.compile(r"^[0-9A-Fa-f]+$")


def _load_env(path):
    values = {}
    if not os.path.isfile(path):
        return values
    with open(path, "r") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, _, val = line.partition("=")
            values[key.strip()] = val.strip().strip('"').strip("'")
    return values


def main():
    rel = os.path.relpath(ENV_PATH, REPO_ROOT)
    values = _load_env(ENV_PATH)
    if not values:
        print("inject_credentials: %s not found or empty "
              "-> using placeholder keys (gates 6/7/9 will not join)" % rel)
        return

    flags = []
    for env_key, (macro, hexlen) in FIELDS.items():
        val = values.get(env_key, "").strip()
        if not val:
            continue
        val = val.upper()
        if not _HEX_RE.match(val) or len(val) != hexlen:
            raise SystemExit(
                "inject_credentials: %s must be %d hex chars (got %r)"
                % (env_key, hexlen, val)
            )
        flags.append((macro, val))

    for macro, val in flags:
        env.Append(CPPDEFINES=[(macro, env.StringifyMacro(val))])  # noqa: F821

    if flags:
        print("inject_credentials: injected %s from %s"
              % (", ".join(m for m, _ in flags), rel))


main()
