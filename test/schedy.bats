#!/bin/bash

setup() {
    bats_load_library bats-support
    bats_load_library bats-assert
    bats_load_library bats-file
    PATH="$BATS_TEST_DIRNAME/..:$PATH"
    cd "$BATS_TEST_TMPDIR" || exit 1
    PIDS="$BATS_FILE_TMPDIR/pids"
    touch "$PIDS"
}

teardown() {
    while read -r pid; do
        echo "$pid"
        kill -s SIGTERM "$pid" 2>/dev/null
        if ! kill -0 "$pid" 2>/dev/null; then continue; fi
        sleep 0.01
        kill -s SIGKILL "$pid"
    done <"$PIDS"
}

peek() {
    timeout -t 500 cat "$1"
}

catnl() {
    cat "$@"
    echo
}

schedy_spawn() {
    run daemonize -i stdin -o stdout -e stderr -p pid schedy
    catnl pid >>"$PIDS"
    assert_success

    # Keep fifos open
    exec 7<>stdin
    exec 8<stdout
    exec 9<stderr

    run ps -p "$(cat pid)"
    assert_success
}

schedy_kill() {
    run kill -s SIGTERM "$(cat pid)"
    assert_success
}

@test "schedy help" {
    schedy --help
}

@test "pooch help" {
    pooch --help
}

ensure_PF02545_hmm() {
    pooch https://pub.danilohorta.me/deciphon/PF02545.hmm --hash ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68
}

@test "schedy daemon" {
    schedy_spawn

    echo "invalid" >stdin
    run peek stdout
    assert_output "FAIL"

    echo "setup http://127.0.0.1:49329 change-me" >stdin
    run peek stdout
    assert_output "OK"

    echo "online" >stdin
    run peek stdout
    assert_output "YES"

    ensure_PF02545_hmm
    assert_file_exists "PF02545.hmm"

    echo "wipe" >stdin
    run peek stdout
    assert_output "OK"

    echo "hmm_up PF02545.hmm" >stdin
    run peek stdout
    assert_output "OK"

    schedy_kill
}
