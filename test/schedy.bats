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
        kill -s SIGKILL "$pid" 2>/dev/null
    done <"$PIDS"
}

peek() {
    timeout -t 500 cat "$1"
}

catnl() {
    cat "$@"
    echo
}

checksum() {
    file=$1
    sha256sum "$file" | cut -f1 -d' '
}

schedy_spawn() {
    run daemonize -i stdin -o stdout -e stderr -p pid schedy -- -u http://127.0.0.1:49329 -k change-me
    assert_success
    catnl pid >>"$PIDS"

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

ensure_PF02545_hmm() {
    python3 -m pipx run pooch-cli https://pub.danilohorta.me/deciphon/PF02545.hmm --hash ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68
}

@test "schedy daemon" {
    schedy_spawn

    echo "online | {1}" >stdin
    run peek stdout
    assert_output "yes"

    ensure_PF02545_hmm
    assert_file_exists "PF02545.hmm"

    echo "wipe | {1}" >stdin
    run peek stdout
    assert_output "ok"

    echo "hmm_up PF02545.hmm | {1}" >stdin
    run peek stdout
    assert_output "ok"

    echo "hmm_dl -7843725841264658444 output.hmm" >stdin
    sleep 1
    assert_file_exists output.hmm
    run checksum output.hmm
    assert_output "ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68"

    schedy_kill
}
