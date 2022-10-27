#!/bin/bash

setup() {
    bats_load_library bats-support
    bats_load_library bats-assert
    bats_load_library bats-file
    load helper
    PATH="$BATS_TEST_DIRNAME/..:$PATH"
    helper_init
    daemon_spawn scanny
    download minifam.dcp
    download consensus.json
    download prods_file_20221021.tsv
}

teardown() {
    daemon_kill
    helper_teardown
}

@test "idle state" {
    run sendo "state | {1} {2}"
    assert_output "ok idle"
}

@test "submit" {
    run sendo "scan consensus.json minifam.dcp prods_file.tsv 1 0 | {1}"
    assert_output "ok"
}

@test "done state" {
    send "scan consensus.json minifam.dcp prods_file.tsv 1 0"
    sleep 2
    run sendo "state | {1} {2}"
    assert_output "ok done"
}

@test "check product file" {
    send "scan consensus.json minifam.dcp prods_file.tsv 1 0"
    wait_file prods_file.tsv
    run diff prods_file.tsv prods_file_20221021.tsv
    assert_success
}
