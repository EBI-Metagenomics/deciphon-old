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

@test "schedy help" {
    schedy --help
}

@test "schedy daemon" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68

    run send "online | {1}"
    assert_output "yes"

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run sendw 0.5 "hmm_dl -7843725841264658444 output.hmm"
    assert_file_exists output.hmm

    run checksum output.hmm
    assert_output ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68

    daemon_kill
}
