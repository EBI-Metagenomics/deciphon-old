#!/bin/bash

setup() {
    load '/Users/horta/code/deciphon/test/test_helper/bats-support/load'
    load '/Users/horta/code/deciphon/test/test_helper/bats-assert/load'
    PATH="$BATS_TEST_DIRNAME/../build:$PATH"
    TIMEOUT=0.01

    # exec 7>&-
    # exec 8>&-
    # exec 9>&-
}

# teardown() {
#     exec 7>&-
#     exec 8>&-
#     exec 9>&-
# }

snore() {
    sleep $TIMEOUT
}

peek() {
    timeout $TIMEOUT cat "$1"
}

keep_fifos_open() {
    exec 7<>stdin
    exec 8<stdout
    exec 9<stderr
}

@test "schedy help" {
    schedy --help
}

@test "schedy daemon" {
    cd "$BATS_TEST_TMPDIR"

    run daemonize schedy
    assert_success

    # snore
    keep_fifos_open

    # snore
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
