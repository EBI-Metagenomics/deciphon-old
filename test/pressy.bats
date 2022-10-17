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

pressy_spawn() {
    run daemonize -i stdin -o stdout -e stderr -p pid pressy
    catnl pid >>"$PIDS"
    assert_success

    # Keep fifos open
    exec 7<>stdin
    exec 8<stdout
    exec 9<stderr

    run ps -p "$(cat pid)"
    assert_success
}

pressy_kill() {
    run kill -s SIGTERM "$(cat pid)"
    assert_success
}

@test "pressy help" {
    pressy --help
}

ensure_PF02545_hmm() {
    python3 -m pipx run pooch-cli https://pub.danilohorta.me/deciphon/PF02545.hmm --hash ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68
}

checksum() {
    file=$1
    sha256sum "$file" | cut -f1 -d' '
}

@test "pressy daemon" {
    pressy_spawn

    ensure_PF02545_hmm
    assert_file_exists "PF02545.hmm"

    echo "press PF02545.hmm | {1} PF02545.hmm" >stdin
    run peek stdout
    assert_output "ok PF02545.hmm"
    assert_file_exists "PF02545.dcp"

    pressy_kill
}
