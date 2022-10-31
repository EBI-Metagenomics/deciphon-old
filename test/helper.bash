#!/bin/bash

helper_init() {
    pipx install pooch-cli >/dev/null
    cd "$BATS_TEST_TMPDIR" || exit 1
    touch pidfile
}

helper_teardown() {
    pidfile_kill
}

pidfile_kill() {
    while read -r pid; do
        kill -s SIGTERM "$pid" 2>/dev/null
        if ! kill -0 "$pid" 2>/dev/null; then continue; fi
        sleep 0.01
        kill -s SIGKILL "$pid" 2>/dev/null || true
    done <pidfile
}

pidfile_store() {
    echo "$1" >>pidfile
}

_daemonize() {
    local -r cmd=$1
    shift
    daemonize -i stdin -o stdout -e stderr -p pid "$cmd" -- "$@"
}

daemon_spawn() {
    _daemonize "$@"
    pidfile_store "$(cat pid)"
    rm pid

    # Keep fifos open
    exec 7<>stdin
    exec 8<stdout
    tail -F stderr >stderr.log &
    pidfile_store $!
}

daemon_kill() {
    pidfile_kill
}

checksum() {
    local -r file=$1
    local cmd=""
    if which sha256sum >/dev/null; then
        cmd="sha256sum"
    elif which shasum >/dev/null; then
        cmd="shasum -a 256"
    else
        exit 1
    fi
    $cmd "$file" | cut -d' ' -f1
}

fetch_hash() {
    local -r file=$1
    local hash
    local f

    while read -r line; do
        hash=$(echo "$line" | cut -f1 -d' ')
        f=$(echo "$line" | cut -f2 -d' ')
        if [ "$file" = "$f" ]; then
            echo "$hash"
            return 0
        fi
    done <"$BATS_TEST_DIRNAME/manifest"

    echo "Filename <$file> is not in the manifest file." >&2
    return 1
}

download() {
    local -r file=$1
    local -r hash=$(fetch_hash "$file")
    pooch https://pub.danilohorta.me/deciphon/"$file" --hash "$hash"
}

recv() {
    local cnt=50
    local output=
    while [ $cnt -gt 0 ]; do
        output=$(timeout -t 100 cat "$1")
        if [ -n "$output" ]; then
            break
        fi
        cnt=$((cnt - 1))
    done
    echo "$output"
}

send() {
    echo "$1" >stdin
}

sendo() {
    echo "$1" >stdin
    recv stdout
}

sendw() {
    echo "$2" >stdin
    sleep "$1"
}

wait_file_exists() {
    local -r file=$1
    local cnt=50
    while [ $cnt -gt 0 ]; do
        sleep 0.1
        if test -e "$file"; then
            break
        fi
        cnt=$((cnt - 1))
    done
}

wait_file_stabilize() {
    local -r file=$1
    local timestamp=
    timestamp=$(date -r "$file" +%s)

    local cnt=5
    while [ $cnt -gt 0 ]; do
        sleep 1.1
        now=$(date -r "$file" +%s)
        if [ "$timestamp" = "$now" ]; then
            break
        fi
        now=$timestamp
        cnt=$((cnt - 1))
    done
}

wait_file() {
    wait_file_exists "$1" && wait_file_stabilize "$1"
}
