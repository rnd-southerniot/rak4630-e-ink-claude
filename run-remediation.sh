#!/usr/bin/env bash
#
# run-remediation.sh — phase-gated driver for the rak4630-e-ink-claude
# code-review remediation.
#
# Walks Phases 0..5 of docs/REMEDIATION_RUNBOOK.md. For each phase it seeds an
# interactive Claude Code session with the phase prompt, then stops and waits
# for YOU to confirm the Smoke Test passed before moving on. It never pushes to
# remote and never opens PRs — at the end it prints the exact commands for you
# to run yourself.
#
# Usage:
#   ./run-remediation.sh                 # run Phase 0 check, then Phases 1..5
#   ./run-remediation.sh --from 3        # start at Phase 3 (skip 1,2)
#   ./run-remediation.sh --only 4        # run a single phase
#   ./run-remediation.sh --print-only    # print the prompts, launch nothing
#   ./run-remediation.sh --accept-edits  # pass --permission-mode acceptEdits to claude
#   ./run-remediation.sh --list          # list phases and exit
#
# Env overrides:
#   RUNBOOK=docs/REMEDIATION_RUNBOOK.md  # path to the runbook (repo-relative)
#   CLAUDE_BIN=claude                    # Claude Code executable
#
# Notes:
#   * Each phase branches off main; the driver returns you to a clean main
#     between phases. A dirty tree halts the run so nothing is clobbered.
#   * Phase 5's hardware gates (2.1, 4) need the physical boards and are run by
#     you on the bench — the driver only covers the desk-side build.
#
set -uo pipefail   # intentionally NOT -e: this is an interactive control loop;
                   # claude's exit code and read prompts are handled explicitly.

# ----------------------------------------------------------------------------
# Config
# ----------------------------------------------------------------------------
RUNBOOK="${RUNBOOK:-docs/REMEDIATION_RUNBOOK.md}"
CLAUDE_BIN="${CLAUDE_BIN:-claude}"

# Phase metadata (index = phase number). Index 0 unused for branch/title.
PHASE_TITLES=(
  "preconditions"
  "CI + gitleaks"
  "encoder temp clamp + tests"
  "README + build-tree reconciliation"
  "security hardening (MCP / context7 / AppKey)"
  "i2c guard + registry BMP280 bounds"
)
PHASE_BRANCHES=(
  ""
  "ci/host-tests-and-gitleaks"
  "fix/encoder-temp-clamp"
  "docs/reconcile-build-trees"
  "security/mcp-and-provisioning-hardening"
  "fix/i2c-len-guard-and-sensor-bounds"
)
PHASE_PR_TITLES=(
  ""
  "ci: gate PRs on host tests + gitleaks secret scan"
  "fix(payload): clamp temperature encoding, add edge-case tests"
  "docs: reconcile build-tree reality (pio active, firmware reference)"
  "security: MCP traversal guard + context7 pin + AppKey log masking"
  "fix(i2c,sensor): 255-byte i2c guard + registry BMP280 bounds"
)
LAST_PHASE=5

CLAUDE_EXTRA_ARGS=()
FROM=1
ONLY=""
PRINT_ONLY=false
DO_LIST=false

# ----------------------------------------------------------------------------
# Pretty output (respects NO_COLOR and non-TTY)
# ----------------------------------------------------------------------------
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
  C_BOLD=$'\033[1m'; C_DIM=$'\033[2m'; C_GRN=$'\033[32m'
  C_YEL=$'\033[33m'; C_RED=$'\033[31m'; C_CYN=$'\033[36m'; C_RST=$'\033[0m'
else
  C_BOLD=""; C_DIM=""; C_GRN=""; C_YEL=""; C_RED=""; C_CYN=""; C_RST=""
fi
info()  { printf '%s[run]%s %s\n'  "$C_CYN" "$C_RST" "$*"; }
ok()    { printf '%s[ ok]%s %s\n'  "$C_GRN" "$C_RST" "$*"; }
warn()  { printf '%s[warn]%s %s\n' "$C_YEL" "$C_RST" "$*" >&2; }
err()   { printf '%s[err]%s %s\n'  "$C_RED" "$C_RST" "$*" >&2; }
rule()  { printf '%s%s%s\n' "$C_DIM" "------------------------------------------------------------" "$C_RST"; }

die() { err "$*"; exit 1; }

usage() { sed -n '3,32p' "$0" | sed 's/^# \{0,1\}//'; exit "${1:-0}"; }

# ----------------------------------------------------------------------------
# Arg parsing
# ----------------------------------------------------------------------------
while [ $# -gt 0 ]; do
  case "$1" in
    --from)        FROM="${2:?--from needs a phase number}"; shift 2 ;;
    --only)        ONLY="${2:?--only needs a phase number}"; shift 2 ;;
    --print-only|--dry-run) PRINT_ONLY=true; shift ;;
    --accept-edits) CLAUDE_EXTRA_ARGS+=(--permission-mode acceptEdits); shift ;;
    --list)        DO_LIST=true; shift ;;
    -h|--help)     usage 0 ;;
    *)             die "unknown argument: $1 (try --help)" ;;
  esac
done

# Soft guard: this remediation edits security-sensitive code; discourage
# blanket permission bypass.
if printf '%s ' "${CLAUDE_EXTRA_ARGS[@]:-}" | grep -qiE 'bypassPermissions|dangerously'; then
  warn "You are running Claude Code with permission bypass on a security remediation."
  warn "Consider plan mode (Shift+Tab) + --accept-edits instead."
fi

# ----------------------------------------------------------------------------
# Per-phase prompt
# ----------------------------------------------------------------------------
phase_prompt() {
  local n="$1"
  local common="Read ./${RUNBOOK} for full context. Make ONLY the changes that phase specifies. Run that phase's verification commands, then STOP at its Smoke Test and report every row as PASS or FAIL. Do NOT start the next phase. Do NOT push to remote or open a PR — stop at 'git commit'."
  case "$n" in
    1) cat <<EOF
Execute **Phase 1 only** (CI + gitleaks) from ./${RUNBOOK}.
Create branch ${PHASE_BRANCHES[1]} off the latest main. Add .github/workflows/ci.yml
and .gitleaks.toml exactly as specified, then run the Phase 1 local verification
(gitleaks dry-run + host tests + YAML parse).
IMPORTANT: if gitleaks reports a finding in git HISTORY, STOP and report it — do
not attempt rotation or history rewrite (that is out of scope, Appendix B).
${common}
EOF
      ;;
    2) cat <<EOF
Execute **Phase 2 only** (encoder temperature clamp + edge-case tests) from ./${RUNBOOK}.
Branch fix/encoder-temp-clamp off main. Add the clamp_temp_centi helper and use it
in v1 and v2 in BOTH pio/src/app_payload.c and firmware/main/app_payload.c, then add
the four byte-verified test vectors to tests/host/payload_encode_test.c and run
tests/host/run_tests.sh.
${common}
EOF
      ;;
    3) cat <<EOF
Execute **Phase 3 only** (README + build-tree reconciliation) from ./${RUNBOOK}.
Branch docs/reconcile-build-trees off main. This is DOCS-ONLY: edit only README.md
and firmware/README.md. Verify the diff touches no code files.
${common}
EOF
      ;;
    4) cat <<EOF
Execute **Phase 4 only** (security hardening) from ./${RUNBOOK}.
Branch security/mcp-and-provisioning-hardening off main. Harden get_doc() in
mcp/firmware-knowledge/server.py, mask the AppKey in tools/provision-node.sh, and
pin context7 in .mcp.json. For the pin, resolve the version with
'npm view @upstash/context7-mcp version' and pin THAT exact version — do not invent
a number. Run the Phase 4 verification block.
${common}
EOF
      ;;
    5) cat <<EOF
Execute **Phase 5 only** (i2c guard + registry BMP280 bounds) from ./${RUNBOOK}.
Branch fix/i2c-len-guard-and-sensor-bounds off main. Add the len>255 guards in
pio/src/i2c_bus.cpp and the plausibility bounds in bmp280_drv_read in
pio/src/sensor_service.cpp, then build BOTH PlatformIO envs (pio run -e rak4631 and
-e rak3312).
The hardware gates (2.1 i2c_smoke, 4 sensor_pipeline) require the physical boards and
are run by ME on the bench — do NOT attempt them. Stop after the build + static checks.
${common}
EOF
      ;;
    *) die "no prompt for phase $n" ;;
  esac
}

# ----------------------------------------------------------------------------
# Git helpers
# ----------------------------------------------------------------------------
git_clean() { [ -z "$(git status --porcelain)" ]; }

ensure_clean_main() {
  local cur; cur="$(git rev-parse --abbrev-ref HEAD)"
  if ! git_clean; then
    err "Working tree is not clean (branch: $cur). Resolve before continuing:"
    git status --short >&2
    return 1
  fi
  if [ "$cur" != "main" ]; then
    info "Switching from '$cur' to main for a clean phase base."
    git switch main >/dev/null 2>&1 || { err "could not switch to main"; return 1; }
  fi
  return 0
}

# ----------------------------------------------------------------------------
# Phase 0 — local preconditions (no Claude needed)
# ----------------------------------------------------------------------------
phase0() {
  rule; info "${C_BOLD}Phase 0${C_RST} — preconditions"
  ensure_clean_main || die "Phase 0 failed: dirty tree."
  if [ -x ./tests/host/run_tests.sh ] || [ -f ./tests/host/run_tests.sh ]; then
    info "Running host unit tests..."
    if bash tests/host/run_tests.sh; then
      ok "Host tests green."
    else
      die "Phase 0 failed: host tests are red on baseline. Fix before remediating."
    fi
  else
    warn "tests/host/run_tests.sh not found — skipping baseline test run."
  fi
  ok "Baseline clean and on main."
}

# ----------------------------------------------------------------------------
# Launch one phase
# ----------------------------------------------------------------------------
launch_phase() {
  local n="$1" prompt rc=0
  prompt="$(phase_prompt "$n")"

  rule
  info "${C_BOLD}Phase $n${C_RST} — ${PHASE_TITLES[$n]}   (branch: ${PHASE_BRANCHES[$n]})"
  rule

  if $PRINT_ONLY; then
    printf '%s\n' "$prompt"
    return 0
  fi

  ensure_clean_main || return 1

  if [ "$n" = "5" ]; then
    warn "Phase 5: hardware gates (2.1, 4) are NOT run here — bench them yourself after the build."
  fi

  info "Tip: press Shift+Tab in the session to enter plan mode, review, then accept."
  info "Launching Claude Code… end the session with /exit (or Ctrl-D) to return here."
  printf '\n'

  if [ "${#CLAUDE_EXTRA_ARGS[@]}" -gt 0 ]; then
    "$CLAUDE_BIN" "${CLAUDE_EXTRA_ARGS[@]}" "$prompt" || rc=$?
  else
    "$CLAUDE_BIN" "$prompt" || rc=$?
  fi
  printf '\n'
  [ "$rc" -ne 0 ] && warn "claude exited with code $rc (expected if you quit the session)."
  return 0
}

# ----------------------------------------------------------------------------
# Gate confirmation between phases
# ----------------------------------------------------------------------------
# Returns: 0 = continue, 1 = retry, 2 = skip, 3 = quit
gate_confirm() {
  local n="$1" ans
  while true; do
    printf '%sGate — Phase %s:%s did the Smoke Test PASS? [c]ontinue / [r]etry / [s]kip / [q]uit: ' \
      "$C_BOLD" "$n" "$C_RST"
    if ! read -r ans; then ans="q"; fi   # EOF -> quit
    case "$ans" in
      c|C|y|Y|"") return 0 ;;
      r|R)        return 1 ;;
      s|S)        return 2 ;;
      q|Q)        return 3 ;;
      *)          warn "Please answer c, r, s, or q." ;;
    esac
  done
}

# ----------------------------------------------------------------------------
# Final summary: print (do not run) push/PR commands for completed phases
# ----------------------------------------------------------------------------
print_pr_commands() {
  local completed=("$@")
  [ "${#completed[@]}" -eq 0 ] && return 0
  rule
  info "${C_BOLD}Next actions${C_RST} — review each branch, then push + open PRs yourself:"
  printf '\n'
  local n
  for n in "${completed[@]}"; do
    printf '  %s# Phase %s — %s%s\n' "$C_DIM" "$n" "${PHASE_TITLES[$n]}" "$C_RST"
    printf '  git switch %s && git log --oneline -1\n' "${PHASE_BRANCHES[$n]}"
    printf '  git push -u origin %s\n' "${PHASE_BRANCHES[$n]}"
    printf '  gh pr create --base main --head %s \\\n' "${PHASE_BRANCHES[$n]}"
    printf '    --title %s \\\n' "\"${PHASE_PR_TITLES[$n]}\""
    printf '    --body  %s\n\n' "\"See ${RUNBOOK} — Phase ${n}.\""
  done
  warn "Nothing was pushed automatically (no-push gate). Merge Phase 1 first so its CI gates the rest."
}

# ----------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------
main() {
  command -v git >/dev/null 2>&1 || die "git not found on PATH."
  local root; root="$(git rev-parse --show-toplevel 2>/dev/null)" \
    || die "Not inside a git repository. cd into the rak4630-e-ink-claude checkout first."
  cd "$root" || die "cannot cd to repo root: $root"

  if $DO_LIST; then
    info "Phases in ${RUNBOOK}:"
    local i
    for i in $(seq 1 "$LAST_PHASE"); do
      printf '  %s. %-44s (branch: %s)\n' "$i" "${PHASE_TITLES[$i]}" "${PHASE_BRANCHES[$i]}"
    done
    exit 0
  fi

  [ -f "$RUNBOOK" ] || die "Runbook not found at ./$RUNBOOK . Set RUNBOOK=... or copy it there.
  Reminder: do NOT name it CLAUDE.md — that clobbers the repo's project-instruction file."

  if ! $PRINT_ONLY; then
    command -v "$CLAUDE_BIN" >/dev/null 2>&1 \
      || die "'$CLAUDE_BIN' not found on PATH. Install Claude Code or set CLAUDE_BIN=."
  fi

  # Resolve which phases to run.
  local -a phases=()
  if [ -n "$ONLY" ]; then
    phases=("$ONLY")
  else
    local i
    for i in $(seq "$FROM" "$LAST_PHASE"); do phases+=("$i"); done
    # Phase 0 baseline only when starting from the top and not print-only.
    if [ "$FROM" -le 1 ] && ! $PRINT_ONLY; then phase0; fi
  fi

  local -a completed=()
  local p
  for p in "${phases[@]}"; do
    if ! { [ "$p" -ge 1 ] && [ "$p" -le "$LAST_PHASE" ]; }; then
      die "invalid phase: $p"
    fi

    while true; do
      launch_phase "$p" || die "Phase $p halted (dirty tree or git error). Resolve and re-run with --from $p."
      $PRINT_ONLY && break   # print-only: no gate, just move on

      gate_confirm "$p"; local g=$?
      case "$g" in
        0) ok "Phase $p accepted."; completed+=("$p"); break ;;
        1) warn "Retrying Phase $p…"; continue ;;
        2) warn "Skipping Phase $p."; break ;;
        3) warn "Quit requested."; print_pr_commands ${completed[@]+"${completed[@]}"}; exit 0 ;;
      esac
    done
  done

  if $PRINT_ONLY; then
    exit 0
  fi

  rule
  ok "Run complete. Phases accepted: ${completed[*]:-none}"
  print_pr_commands ${completed[@]+"${completed[@]}"}
}

main "$@"
