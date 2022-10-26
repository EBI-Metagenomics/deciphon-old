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
    exec 9<stderr
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

download() {
    local -r file=$1
    local -r hash=$2
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
    recv stdout
}

sendw() {
    echo "$2" >stdin
    sleep "$1"
}
