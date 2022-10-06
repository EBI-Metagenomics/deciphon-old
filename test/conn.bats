#!/bin/bash

setup() {
    bats_load_library bats-support
    bats_load_library bats-assert
    bats_load_library bats-file
    PATH="$BATS_TEST_DIRNAME/../build:$PATH"
    cd "$BATS_TEST_TMPDIR" || exit 1
    PIDS="$BATS_FILE_TMPDIR/pids"
    touch "$PIDS"
}

teardown() {
    while read -r pid; do
        echo "$pid"
        kill -s SIGTERM "$pid"
        if kill -0 "$pid"; then continue; fi
        sleep 0.01
        kill -s SIGKILL "$pid"
    done <"$PIDS"
}

peek() {
    timeout 0.01 cat "$1"
}

keep_fifos_open() {
    exec 7<>stdin
    exec 8<stdout
    exec 9<stderr
}

catnl() {
    cat "$@"
    echo
}

@test "schedy help" {
    schedy --help
}

@test "schedy daemon" {
    run daemonize schedy
    assert_file_exists "$PIDS"
    catnl pid >>"$PIDS"
    assert_success
    keep_fifos_open

    run ps -p "$(cat pid)"
    assert_success

    echo "invalid" >stdin
    run peek stdout
    assert_output "FAIL"

    echo "setup http://127.0.0.1:49329 change-me" >stdin
    run peek stdout
    assert_output "OK"

    echo "online" >stdin
    run peek stdout
    assert_output "YES"

    run kill -2 "$(cat pid)"
    assert_success
}
