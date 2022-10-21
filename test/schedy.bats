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

@test "schedy hmm" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68
    download PF02545.dcp 62f3961caa6580baf68126947031af16cff90ae7f6ec0a0ec0f7b2d7950da8e1

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

    run send "hmm_get_by_id 1 | {1} {2}"
    assert_output 'ok {"id":1,"xxh3":-7843725841264658444,"filename":"PF02545.hmm","job_id":1}'

    run send "hmm_get_by_xxh3 -7843725841264658444 | {1} {2}"
    assert_output 'ok {"id":1,"xxh3":-7843725841264658444,"filename":"PF02545.hmm","job_id":1}'

    run send "hmm_get_by_job_id 1 | {1} {2}"
    assert_output 'ok {"id":1,"xxh3":-7843725841264658444,"filename":"PF02545.hmm","job_id":1}'

    run send "hmm_get_by_filename PF02545.hmm | {1} {2}"
    assert_output 'ok {"id":1,"xxh3":-7843725841264658444,"filename":"PF02545.hmm","job_id":1}'

    run send "db_up PF02545.dcp | {1}"
    assert_output "ok"

    run send "db_get_by_id 1 | {1} {2}"
    assert_output 'ok {"id":1,"xxh3":2407434389743836934,"filename":"PF02545.dcp","hmm_id":1}}'

    run send "db_get_by_xxh3 2407434389743836934 | {1} {2}"
    assert_output 'ok {"id":1,"xxh3":2407434389743836934,"filename":"PF02545.dcp","hmm_id":1}}'

    run send "db_get_by_hmm_id 1 | {1} {2}"
    assert_output 'ok {"id":1,"xxh3":2407434389743836934,"filename":"PF02545.dcp","hmm_id":1}}'

    run send "db_get_by_filename PF02545.dcp | {1} {2}"
    assert_output 'ok {"id":1,"xxh3":2407434389743836934,"filename":"PF02545.dcp","hmm_id":1}}'

    daemon_kill
}

@test "schedy job" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68
    download PF02545.dcp 62f3961caa6580baf68126947031af16cff90ae7f6ec0a0ec0f7b2d7950da8e1

    run send "online | {1}"
    assert_output "yes"

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run sendw 0.5 "hmm_dl -7843725841264658444 output.hmm"
    assert_file_exists output.hmm

    run send "job_next_pend | {1} {2}"
    assert_output -e 'ok {"id":1,"type":1,"state":"pend","progress":0,"error":"","submission":-?[0-9]+,"exec_started":0,"exec_ended":0}'

    daemon_kill
}
