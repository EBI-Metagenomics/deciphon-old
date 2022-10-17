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

scanny_spawn() {
    run daemonize -i stdin -o stdout -e stderr -p pid scanny
    catnl pid >>"$PIDS"
    assert_success

    # Keep fifos open
    exec 7<>stdin
    exec 8<stdout
    exec 9<stderr

    run ps -p "$(cat pid)"
    assert_success
}

scanny_kill() {
    run kill -s SIGTERM "$(cat pid)"
    assert_success
}

@test "scanny help" {
    scanny --help
}

ensure_PF02545_dcp() {
    pipx run pooch-cli https://pub.danilohorta.me/deciphon/PF02545.dcp --hash 62f3961caa6580baf68126947031af16cff90ae7f6ec0a0ec0f7b2d7950da8e1
}

ensure_consensus_faa() {
    pipx run pooch-cli https://pub.danilohorta.me/deciphon/consensus.faa --hash dd8fc237e249f550890ec58e64bc4aa31fd708e341f1cb14a6800bee27fd8962
}

checksum() {
    file=$1
    sha256sum "$file" | cut -f1 -d' '
}

@test "scanny daemon" {
    scanny_spawn

    ensure_PF02545_dcp
    assert_file_exists "PF02545.dcp"

    ensure_consensus_faa
    assert_file_exists "consensus.faa"

    # echo "press PF02545.hmm | ack {1} PF02545.hmm" >stdin
    # run peek stdout
    # assert_output "ack ok PF02545.hmm"
    # assert_file_exists "PF02545.dcp"

    scanny_kill
}
