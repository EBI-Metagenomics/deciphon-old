#!/bin/bash

setup() {
    bats_load_library bats-support
    bats_load_library bats-assert
    bats_load_library bats-file
    load helper
    PATH="$BATS_TEST_DIRNAME/..:$PATH"
    cd "$BATS_TEST_TMPDIR" || exit 1
    PIDS="$BATS_FILE_TMPDIR/pids"
    touch "$PIDS"
    pipx install pooch-cli
}

teardown() {
    while read -r pid; do
        echo "$pid"
        kill -s SIGTERM "$pid" 2>/dev/null
        if ! kill -0 "$pid" 2>/dev/null; then continue; fi
        sleep 0.01
        kill -s SIGKILL "$pid" 2>/dev/null
    done <"$PIDS"
}

schedy_spawn() {
    run daemonize -i stdin -o stdout -e stderr -p pid schedy -- -u http://127.0.0.1:49329 -k change-me
    assert_success
    cat pid >>"$PIDS"
    echo >>"$PIDS"

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

@test "schedy daemon" {
    schedy_spawn
    download PF02545.hmm ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68

    run send "online | {1}"
    assert_output "yes"

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send_nb "hmm_dl -7843725841264658444 output.hmm"
    sleep 0.5
    assert_file_exists output.hmm

    run checksum output.hmm
    assert_output ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68

    schedy_kill
}
