#!/usr/bin/env bash
set -euo pipefail

workspace="${GITHUB_WORKSPACE:-$(pwd)}"
rtthread_ref="${RTTHREAD_REF:-master}"
rtthread_resolved_sha="${RTTHREAD_RESOLVED_SHA:-}"
ci_root="$workspace/_ci"
out_dir="$ci_root/rt-thread-source"
source_dir="$out_dir/rt-thread"
tar_file="$out_dir/rt-thread-source.tar"
sha_file="$out_dir/rt-thread-source.sha"
profile_file="$workspace/.github/ci/rtthread-profile-list.txt"
log_file="$out_dir/prepare-rtthread-source.log"
cache_dir="${RTTHREAD_SOURCE_CACHE_DIR:-}"
cache_tar_file=""
cache_sha_file=""

mkdir -p "$out_dir"
: > "$log_file"

run_logged() {
    echo "+ $*" | tee -a "$log_file"
    "$@" 2>&1 | tee -a "$log_file"
}

json_escape_string() {
    local value="$1"

    value="${value//\\/\\\\}"
    value="${value//\"/\\\"}"
    printf '%s' "$value"
}

configure_cache_paths() {
    if [ -z "$cache_dir" ]; then
        return 0
    fi

    mkdir -p "$cache_dir"
    cache_tar_file="$cache_dir/rt-thread-source.tar"
    cache_sha_file="$cache_dir/rt-thread-source.sha"
}

cache_is_valid() {
    test -n "$rtthread_resolved_sha" || return 1
    test -n "$cache_tar_file" || return 1
    test -s "$cache_tar_file" || return 1
    test -s "$cache_sha_file" || return 1

    if [ -n "$rtthread_resolved_sha" ]; then
        case "$(cat "$cache_sha_file")" in
            "$rtthread_resolved_sha"*) ;;
            *) return 1 ;;
        esac
    fi

    return 0
}

restore_from_cache() {
    cp "$cache_tar_file" "$tar_file"
    cp "$cache_sha_file" "$sha_file"
    echo "RTTHREAD_SOURCE_CACHE_HIT ref=$rtthread_ref sha=$(cat "$sha_file")" | tee -a "$log_file"
}

store_to_cache() {
    if [ -z "$cache_tar_file" ]; then
        return 0
    fi

    cp "$tar_file" "$cache_tar_file"
    cp "$sha_file" "$cache_sha_file"
    echo "RTTHREAD_SOURCE_CACHE_STORE ref=$rtthread_ref sha=$(cat "$sha_file") dir=$cache_dir" | tee -a "$log_file"
}

retry_clone_rtthread() {
    local attempt=1
    local max_attempts=3

    while [ "$attempt" -le "$max_attempts" ]; do
        if run_logged git clone --depth 1 --branch "$rtthread_ref" \
            https://github.com/RT-Thread/rt-thread.git "$source_dir"; then
            return 0
        fi

        rm -rf "$source_dir"
        if run_logged git init "$source_dir" && \
            run_logged git -C "$source_dir" remote add origin https://github.com/RT-Thread/rt-thread.git && \
            run_logged git -C "$source_dir" fetch --depth 1 origin "$rtthread_ref" && \
            run_logged git -C "$source_dir" checkout --detach FETCH_HEAD; then
            return 0
        fi

        rm -rf "$source_dir"
        if [ "$attempt" -eq "$max_attempts" ]; then
            break
        fi

        sleep_time=$((attempt * 10))
        echo "Retry RT-Thread clone in ${sleep_time}s (${attempt}/${max_attempts})" | tee -a "$log_file"
        sleep "$sleep_time"
        attempt=$((attempt + 1))
    done

    return 1
}

emit_profile_output() {
    local first=1
    local json="["
    local profile

    test -f "$profile_file" || {
        echo "Profile list is missing: $profile_file" >&2
        exit 1
    }

    while IFS= read -r profile; do
        profile="${profile%$'\r'}"
        case "$profile" in
            ''|'#'*)
                continue
                ;;
        esac
        if [ "$first" -eq 0 ]; then
            json="${json},"
        fi
        json="${json}\"$(json_escape_string "$profile")\""
        first=0
    done < "$profile_file"

    json="${json}]"

    if [ -n "${GITHUB_OUTPUT:-}" ]; then
        printf 'profile-list=%s\n' "$json" >> "$GITHUB_OUTPUT"
    fi
}

emit_rtthread_output() {
    local sha

    sha="$(cat "$sha_file")"
    if [ -n "${GITHUB_OUTPUT:-}" ]; then
        printf 'rtthread-sha=%s\n' "$sha" >> "$GITHUB_OUTPUT"
    fi
}

configure_cache_paths
rm -rf "$source_dir" "$tar_file" "$sha_file"
mkdir -p "$(dirname "$source_dir")"

if cache_is_valid; then
    restore_from_cache
else
    retry_clone_rtthread
    sha="$(git -C "$source_dir" rev-parse HEAD)"
    printf '%s\n' "$sha" > "$sha_file"

    tar --exclude='rt-thread/.git' -cf "$tar_file" -C "$out_dir" rt-thread

    test -s "$tar_file"
    test -s "$sha_file"
    store_to_cache
fi

emit_rtthread_output
emit_profile_output

echo "RTTHREAD_SOURCE_READY ref=$rtthread_ref sha=$(cat "$sha_file") tar=$tar_file" | tee -a "$log_file"
