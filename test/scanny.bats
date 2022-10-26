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

@test "scanny daemon" {
    daemon_spawn scanny
    download minifam.dcp
    download consensus.json
    download prods_file_20221017.tsv

    run send "state | {1} {2}"
    assert_output "ok idle"

    run send "scan consensus.json minifam.dcp prods_file.tsv 1 0 | {1}"
    assert_output "ok"

    sleep 2
    run send "state | {1} {2}"
    assert_output "ok done"

    sort -o prods_file.tsv prods_file.tsv
    run diff prods_file.tsv prods_file_20221017.tsv
    assert_output ""

    daemon_kill
}
