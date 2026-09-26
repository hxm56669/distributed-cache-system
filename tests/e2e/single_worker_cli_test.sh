#!/usr/bin/env bash
set -euo pipefail

worker=$1
client=$2
port=${3:-50154}
work_dir=$(mktemp -d /tmp/storage-v1-cli-e2e.XXXXXX)
worker_pid=""

cleanup() {
    if [[ -n "$worker_pid" ]]; then
        kill "$worker_pid" 2>/dev/null || true
        wait "$worker_pid" 2>/dev/null || true
    fi
    rm -rf "$work_dir"
}
trap cleanup EXIT INT TERM

truncate -s 8M "$work_dir/source.bin"
"$worker" --listen "127.0.0.1:$port" --storage-root "$work_dir/chunks" \
    >"$work_dir/worker.log" 2>&1 &
worker_pid=$!
sleep 1

"$client" --worker-data-port "$port" put cli-e2e "$work_dir/source.bin"
"$client" --worker-data-port "$port" head cli-e2e
"$client" --worker-data-port "$port" get cli-e2e "$work_dir/download.bin"
cmp "$work_dir/source.bin" "$work_dir/download.bin"
"$client" --worker-data-port "$port" delete cli-e2e
if "$client" --worker-data-port "$port" head cli-e2e >/dev/null 2>&1; then
    echo "deleted object remained visible" >&2
    exit 1
fi
