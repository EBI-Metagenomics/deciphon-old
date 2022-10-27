#!/bin/bash

setup() {
    bats_load_library bats-support
    bats_load_library bats-assert
    bats_load_library bats-file
    load helper
    PATH="$BATS_TEST_DIRNAME/..:$PATH"
    helper_init
    if [ -z ${API_HOST+x} ]; then
        API_HOST=127.0.0.1
    fi
    daemon_spawn schedy -u http://"$API_HOST":49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp
    download consensus.faa
    download prods_file_20221021.tsv
}

teardown() {
    daemon_kill
    helper_teardown
}

@test "echo" {
    run sendo "echo hello world"
    assert_output "echo hello world"
}

@test "online" {
    run sendo "online | {1}"
    assert_output "yes"
}

@test "wipe" {
    run sendo "wipe | {1}"
    assert_output "ok"
}

@test "get non-existent pending job" {
    send "wipe"
    run sendo "job_next_pend | {1}"
    assert_output 'end'
}

@test "upload hmm" {
    send "wipe"
    run sendo "hmm_up PF02545.hmm | {1}"
    assert_output "ok"
}

@test "download hmm" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    send "hmm_dl -7843725841264658444 output.hmm"
    wait_file output.hmm
    run checksum output.hmm
    assert_output ce7760d930dd17efaac841177f33f507e0e3d7e8c0d59f0cb4c058b6659bbd68
}

@test "get non-existent hmm" {
    run sendo "hmm_get_by_id 743982 | {1}"
    assert_output 'fail'
}

@test "get hmm" {
    send "wipe"
    send "hmm_up PF02545.hmm"

    local -r answer='ok {"id":1,"xxh3":-7843725841264658444,"filename":"PF02545.hmm","job_id":1}'
    run sendo "hmm_get_by_id 1 | {1} {2}"
    assert_output "$answer"

    run sendo "hmm_get_by_xxh3 -7843725841264658444 | {1} {2}"
    assert_output "$answer"

    run sendo "hmm_get_by_job_id 1 | {1} {2}"
    assert_output "$answer"

    run sendo "hmm_get_by_filename PF02545.hmm | {1} {2}"
    assert_output "$answer"
}

@test "upload db without hmm" {
    send "wipe"
    run sendo "db_up PF02545.dcp | {1}"
    assert_output "fail"
}

@test "upload db" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    run sendo "db_up PF02545.dcp | {1}"
    assert_output "ok"
}

@test "download db" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    send "db_up PF02545.dcp"
    send "db_dl 2407434389743836934 output.hmm"
    wait_file output.hmm
    run checksum output.hmm
    assert_output 62f3961caa6580baf68126947031af16cff90ae7f6ec0a0ec0f7b2d7950da8e1
}

@test "get non-existent db" {
    run sendo "db_get_by_id 743982 | {1}"
    assert_output 'fail'
}

@test "get db" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    send "db_up PF02545.dcp"

    local -r answer='ok {"id":1,"xxh3":2407434389743836934,"filename":"PF02545.dcp","hmm_id":1}'
    run sendo "db_get_by_id 1 | {1} {2}"
    assert_output "$answer"

    run sendo "db_get_by_xxh3 2407434389743836934 | {1} {2}"
    assert_output "$answer"

    run sendo "db_get_by_hmm_id 1 | {1} {2}"
    assert_output "$answer"

    run sendo "db_get_by_filename PF02545.dcp | {1} {2}"
    assert_output "$answer"
}

@test "get pending press job" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    run sendo "job_next_pend | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'
}

@test "set job state" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    run sendo "job_set_state 1 run | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"run","progress":0,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":0\}'
}

@test "get job state" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    send "job_set_state 1 run"
    run sendo "job_set_state 1 done | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"done","progress":0,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":-?[0-9]+\}'
}

@test "increment progress" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    run sendo "job_inc_progress 1 5 | {1}"
    assert_output "ok"
    run sendo "job_set_state 1 done | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"done","progress":5,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":-?[0-9]+\}'
}

@test "submit scan job" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    send "db_up PF02545.dcp"
    send "job_set_state 1 done"
    run sendo "scan_submit 1 1 0 consensus.faa | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'
}

@test "get pending scan job" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    send "db_up PF02545.dcp"
    send "job_set_state 1 done"
    send "scan_submit 1 1 0 consensus.faa"
    run sendo "job_next_pend | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'
}

@test "get scan job" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    send "db_up PF02545.dcp"
    send "job_set_state 1 done"
    send "scan_submit 1 1 0 consensus.faa"
    send "job_next_pend"
    run sendo "scan_get_by_job_id 2 | {1} {2}"
    assert_output 'ok {"id":1,"db_id":1,"multi_hits":true,"hmmer3_compat":false,"job_id":2}'
}

@test "count scan job sequences" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    send "db_up PF02545.dcp"
    send "job_set_state 1 done"
    send "scan_submit 1 1 0 consensus.faa"
    send "job_next_pend"
    send "scan_get_by_job_id 2"
    run sendo "scan_seq_count 1 | {1} {2}"
    assert_output 'ok 3'
}

@test "download scan job sequences" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    send "db_up PF02545.dcp"
    send "job_set_state 1 done"
    send "scan_submit 1 1 0 consensus.faa"
    send "job_next_pend"
    send "scan_get_by_job_id 2"
    run sendo "scan_dl_seqs 1 seqs.json | {1}"
    assert_output 'ok'
    wait_file seqs.json
    run checksum seqs.json
    assert_output 12ac6942c48d95d34544e07262d30c62d838e7caa93dc4f3fe4e9dd2a2935c0d
}

@test "upload scan job product" {
    send "wipe"
    send "hmm_up PF02545.hmm"
    send "db_up PF02545.dcp"
    send "job_set_state 1 done"
    send "scan_submit 1 1 0 consensus.faa"
    send "job_next_pend"
    send "scan_get_by_job_id 2"
    send "scan_dl_seqs 1 seqs.json"
    run sendo "prods_file_up prods_file_20221021.tsv | {1}"
    assert_output 'ok'
}
