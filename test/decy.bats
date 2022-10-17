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

decy_spawn() {
    run daemonize -i stdin -o stdout -e stderr -p pid decy
    catnl pid >>"$PIDS"
    assert_success

    # Keep fifos open
    exec 7<>stdin
    exec 8<stdout
    exec 9<stderr

    run ps -p "$(cat pid)"
    assert_success
}

decy_kill() {
    run kill -s SIGTERM "$(cat pid)"
    assert_success
}

@test "decy help" {
    decy --help
}

ensure_PF02545_hmm() {
    pipx run pooch-cli https://pub.danilohorta.me/deciphon/PF02545.hmm --hash ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68
}

checksum() {
    file=$1
    sha256sum "$file" | cut -f1 -d' '
}

@test "decy daemon" {
    decy_spawn

    echo "invalid" >stdin
    run peek stdout
    assert_output "FAIL"

    ensure_PF02545_hmm
    assert_file_exists "PF02545.hmm"

    echo "press PF02545.hmm" >stdin
    run peek stdout
    assert_output "OK"
    assert_file_exists "PF02545.dcp"

    run checksum "PF02545.dcp"
    assert_output 62f3961caa6580baf68126947031af16cff90ae7f6ec0a0ec0f7b2d7950da8e1

    decy_kill
}
