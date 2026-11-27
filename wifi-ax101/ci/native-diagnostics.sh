# RUN86 CI-only native-build diagnostics; source inside the OpenIndiana VM shell.
ax101_diag_init() {
    AX101_ART=$1
    mkdir -p "$AX101_ART"
    : > "$AX101_ART/command-journal.log"
    : > "$AX101_ART/stage-status.tsv"
    : > "$AX101_ART/native-stdout.log"
    : > "$AX101_ART/native-stderr.log"
    : > "$AX101_ART/failure-summary.txt"
    AX101_STAGE=VM_ENVIRONMENT_INITIALIZATION
    AX101_LAST_COMMAND='<none>'
    AX101_LAST_RC=0
    for s in VM_ENVIRONMENT_INITIALIZATION PINNED_UPSTREAM_CHECKOUT CANDIDATE_TRANSFER_VERIFICATION PATCH_STACK_APPLICATION INTEGRATION_REPLAY_VALIDATION SOURCE_STATIC_TESTS HOST_MODEL_TESTS NATIVE_BUILD_PREPARATION EXACT_NATIVE_GMAKE NATIVE_COMPILATION NATIVE_LINKING ELF_MODULE_INSPECTION ARTIFACT_PACKAGING; do
        printf '%s\tNOT_RUN\t%s\n' "$s" "$(date -u +%FT%TZ)" >> "$AX101_ART/stage-status.tsv"
    done
    exec > >(tee -a "$AX101_ART/native-openindiana-build.log" "$AX101_ART/native-stdout.log") 2> >(tee -a "$AX101_ART/native-openindiana-build.log" "$AX101_ART/native-stderr.log" >&2)
    exec 9>>"$AX101_ART/command-journal.log"
    BASH_XTRACEFD=9
    PS4='+$(date -u +%FT%TZ) stage=${AX101_STAGE:-UNSET} pid=$$ cwd=$PWD cmd='
    export BASH_XTRACEFD PS4
    set -E
    trap 'ax101_diag_on_error "$?" "$BASH_COMMAND"' ERR
    set -x
}
ax101_diag_stage() {
    AX101_STAGE=$1
    printf '%s\tRUNNING\t%s\n' "$AX101_STAGE" "$(date -u +%FT%TZ)" >> "$AX101_ART/stage-status.tsv"
    printf 'AX101_STAGE=%s STATUS=RUNNING UTC=%s PWD=%s PID=%s\n' "$AX101_STAGE" "$(date -u +%FT%TZ)" "$PWD" "$$"
}
ax101_diag_pass() {
    printf '%s\tPASS\t%s\n' "$1" "$(date -u +%FT%TZ)" >> "$AX101_ART/stage-status.tsv"
    printf 'AX101_STAGE=%s STATUS=PASS\n' "$1"
}
ax101_diag_command() {
    label=$1; shift
    AX101_LAST_COMMAND="$label: $(printf '%q ' "$@")"
    start=$(date +%s)
    printf 'COMMAND_BEGIN utc=%s stage=%s cwd=%s pid=%s command=%s\n' "$(date -u +%FT%TZ)" "$AX101_STAGE" "$PWD" "$$" "$AX101_LAST_COMMAND"
    if "$@"; then rc=0; else rc=$?; fi
    elapsed=$(($(date +%s) - start))
    AX101_LAST_RC=$rc
    printf 'COMMAND_END utc=%s stage=%s command=%s exit_code=%s elapsed_seconds=%s status=%s\n' "$(date -u +%FT%TZ)" "$AX101_STAGE" "$label" "$rc" "$elapsed" "$([ "$rc" -eq 0 ] && echo PASS || echo FAIL)"
    return "$rc"
}
ax101_diag_on_error() {
    rc=$1; cmd=$2
    AX101_LAST_RC=$rc; AX101_LAST_COMMAND=$cmd
    {
        printf 'failure_utc=%s\nstage=%s\nexit_code=%s\nlast_command=%s\ncwd=%s\npid=%s\n' "$(date -u +%FT%TZ)" "$AX101_STAGE" "$rc" "$cmd" "$PWD" "$$"
        printf '%s\n' '--- stdout tail ---'; tail -n 80 "$AX101_ART/native-stdout.log" 2>/dev/null || true
        printf '%s\n' '--- stderr tail ---'; tail -n 80 "$AX101_ART/native-stderr.log" 2>/dev/null || true
    } > "$AX101_ART/failure-summary.txt" 2>&1 || true
    return 0
}
ax101_diag_finish() {
    rc=$1
    set +e
    trap - ERR
    if [ "$rc" -eq 0 ]; then ax101_diag_pass "$AX101_STAGE"; else printf '%s\tFAIL\t%s\n' "$AX101_STAGE" "$(date -u +%FT%TZ)" >> "$AX101_ART/stage-status.tsv"; fi
    {
        printf 'exit_code=%s\nlast_stage=%s\nlast_command=%s\nlast_command_exit_code=%s\ncompleted_utc=%s\n' "$rc" "$AX101_STAGE" "$AX101_LAST_COMMAND" "$AX101_LAST_RC" "$(date -u +%FT%TZ)"
    } > "$AX101_ART/native-build-status.txt" 2>/dev/null || true
    if [ "$rc" -ne 0 ] && [ ! -s "$AX101_ART/failure-summary.txt" ]; then
        printf 'stage=%s\nexit_code=%s\nlast_command=%s\n' "$AX101_STAGE" "$rc" "$AX101_LAST_COMMAND" > "$AX101_ART/failure-summary.txt" 2>/dev/null || true
    fi
    return 0
}
