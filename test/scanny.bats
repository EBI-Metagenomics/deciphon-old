#!/bin/bash

setup() {
    bats_load_library bats-support
    bats_load_library bats-assert
    bats_load_library bats-file
    load helper
    PATH="$BATS_TEST_DIRNAME/..:$PATH"
    helper_init
}

teardown() {
    helper_teardown
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

@test "scanny daemon" {
    daemon_spawn scanny
    download minifam.dcp 40d96b5a62ff669e19571c392ab711c7188dd5490744edf6c66051ecb4f2243d
    download consensus.json af483ed5aa42010e8f6c950c42d81bac69f995876bf78a5965f319e83dc3923e

    # ensure_minifam_dcp
    # assert_file_exists "minifam.dcp"
    #
    # ensure_consensus_json
    # assert_file_exists "consensus.json"
    #
    # ensure_prods_file_20221017_tsv
    # assert_file_exists "prods_file_20221017.tsv"
    #
    # echo "state | {1} {2}" >stdin
    # run peek stdout
    # assert_output "ok IDLE"
    #
    # echo "scan consensus.json minifam.dcp prods_file.tsv 1 0 | {1}" >stdin
    # run peek stdout
    # assert_output "ok"
    #
    # sleep 2
    # echo "state | {1} {2}" >stdin
    # run peek stdout
    # assert_output "ok DONE"
    #
    # run sort -o prods_file.tsv prods_file.tsv
    # run diff prods_file.tsv prods_file_20221017.tsv
    # assert_output ""

    daemon_kill
}
