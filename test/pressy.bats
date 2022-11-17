#!/bin/bash

setup() {
    bats_load_library bats-support
    bats_load_library bats-assert
    bats_load_library bats-file
    load helper
    PATH="$BATS_TEST_DIRNAME/..:$PATH"
    helper_init
    daemon_spawn pressy
    download PF02545.hmm
    download Pfam-A.100.hmm
}

teardown() {
    daemon_kill
    helper_teardown
}

@test "echo" {
    run sendo "echo hello world"
    assert_output "hello world"
}

@test "press" {
    run sendo "press PF02545.hmm | {1}"
    assert_output "ok"
    assert_file_exists "PF02545.dcp"
}

@test "cancel" {
    send "press Pfam-A.100.hmm"
    run sendo "cancel | {1}"
    assert_output "ok"
}

@test "init state" {
    run sendo "state | {1} {2}"
    assert_output "ok init"
}

@test "run state" {
    send "press PF02545.hmm"
    sleep 0.2
    run sendo "state | {1} {2} {3} {4}"
    assert_output -e "ok run [0-9]+% PF02545.hmm"
}

@test "cancel & state" {
    send "press Pfam-A.100.hmm"
    send "cancel"
    sleep 0.2
    run sendo "state | {1} {2}"
    assert_output "ok cancel"
}
