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

ensure_minifam_dcp() {
    python3 -m pipx run pooch-cli https://pub.danilohorta.me/deciphon/minifam.dcp --hash 40d96b5a62ff669e19571c392ab711c7188dd5490744edf6c66051ecb4f2243d
}

ensure_consensus_json() {
    python3 -m pipx run pooch-cli https://pub.danilohorta.me/deciphon/consensus.json --hash af483ed5aa42010e8f6c950c42d81bac69f995876bf78a5965f319e83dc3923e
}

ensure_prods_file_20221017_tsv() {
    python3 -m pipx run pooch-cli https://pub.danilohorta.me/deciphon/prods_file_20221017.tsv --hash 5cfdaf4283ae0801709ce42efd61c1ee06873c20647154140aafd09db9a366a7
}

checksum() {
    file=$1
    sha256sum "$file" | cut -f1 -d' '
}

@test "scanny daemon" {
    scanny_spawn

    ensure_minifam_dcp
    assert_file_exists "minifam.dcp"

    ensure_consensus_json
    assert_file_exists "consensus.json"

    ensure_prods_file_20221017_tsv
    assert_file_exists "prods_file_20221017.tsv"

    echo "state | {1} {2}" >stdin
    run peek stdout
    assert_output "ok IDLE"

    echo "scan consensus.json minifam.dcp prods_file.tsv 1 0 | {1}" >stdin
    run peek stdout
    assert_output "ok"

    sleep 2
    echo "state | {1} {2}" >stdin
    run peek stdout
    assert_output "ok DONE"

    run sort -o prods_file.tsv prods_file.tsv
    run diff prods_file.tsv prods_file_20221017.tsv
    assert_output ""

    scanny_kill
}
