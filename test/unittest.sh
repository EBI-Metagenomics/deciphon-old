#!/bin/bash

unittest_origin=$PWD
cd "$(mktemp -d)" || exit 1
PID=

function unittest_exec() {
    program=$1
    mkfifo stdin
    "$unittest_origin/unittest_run.sh" "$unittest_origin/$program" stdin stdout stderr &
    PID=$!
}

function unittest_stop() {
    sleep 0.5
    { kill -15 "$PID" && wait "$PID"; } 2>/dev/null
    sleep 0.5
}

function unittest_sendnl() {
    echo "$1" >>stdin
}
