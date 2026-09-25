#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
build_dir=${BUILD_DIR:-"$repo_root/build/bench-release"}
output=${1:-"$repo_root/benchmark-results/v1-single-worker/results.csv"}
work_dir=$(mktemp -d /tmp/storage-v1-benchmark.XXXXXX)
worker_pid=""

cleanup() {
    if [[ -n "$worker_pid" ]]; then
        kill "$worker_pid" 2>/dev/null || true
        wait "$worker_pid" 2>/dev/null || true
    fi
    rm -rf "$work_dir"
}
trap cleanup EXIT INT TERM

mkdir -p "$(dirname "$output")"
"$build_dir/worker" --listen 127.0.0.1:50153 --storage-root "$work_dir/chunks" \
    >"$work_dir/worker.log" 2>&1 &
worker_pid=$!
sleep 1

echo "size_bytes,operation,wall_seconds,throughput_mib_s,max_rss_kib,cpu_percent" >"$output"
sizes=${SIZES:-"1048576 67108864 1073741824 5368709120"}
for size in $sizes; do
    source_file="$work_dir/source-$size.bin"
    destination_file="$work_dir/destination-$size.bin"
    object_id="benchmark-$size"
    truncate -s "$size" "$source_file"

    /usr/bin/time -f "%e,%M,%P" -o "$work_dir/time.txt" \
        "$build_dir/client" --worker 127.0.0.1:50153 put "$object_id" "$source_file"
    IFS=, read -r wall rss cpu <"$work_dir/time.txt"
    throughput=$(awk -v bytes="$size" -v seconds="$wall" \
        'BEGIN { if (seconds == 0) seconds = 0.001; printf "%.2f", bytes / 1048576 / seconds }')
    echo "$size,put,$wall,$throughput,$rss,$cpu" >>"$output"

    /usr/bin/time -f "%e,%M,%P" -o "$work_dir/time.txt" \
        "$build_dir/client" --worker 127.0.0.1:50153 get "$object_id" "$destination_file"
    IFS=, read -r wall rss cpu <"$work_dir/time.txt"
    throughput=$(awk -v bytes="$size" -v seconds="$wall" \
        'BEGIN { if (seconds == 0) seconds = 0.001; printf "%.2f", bytes / 1048576 / seconds }')
    echo "$size,get,$wall,$throughput,$rss,$cpu" >>"$output"

    cmp "$source_file" "$destination_file"
    "$build_dir/client" --worker 127.0.0.1:50153 delete "$object_id"
    rm -f "$source_file" "$destination_file"
done
