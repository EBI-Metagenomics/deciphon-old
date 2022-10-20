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

@test "pressy help" {
    pressy --help
}

@test "pressy echo" {
    daemon_spawn pressy
    run send "echo hello world"
    assert_output "echo hello world"
    daemon_kill
}

@test "pressy press" {
    daemon_spawn pressy
    download PF02545.hmm ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68

    run send "press PF02545.hmm | {1} PF02545.hmm"
    assert_output "ok PF02545.hmm"
    assert_file_exists "PF02545.dcp"

    daemon_kill
}
