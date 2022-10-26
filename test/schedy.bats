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

@test "help" {
    schedy --help
}

@test "online" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me

    run send "online | {1}"
    assert_output "yes"

    daemon_kill
}

@test "wipe" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me

    run send "wipe | {1}"
    assert_output "ok"

    daemon_kill
}

@test "get non-existent pending job" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me

    run send "wipe | {1}"
    assert_output "ok"

    run send "job_next_pend | {1}"
    assert_output 'end'

    daemon_kill
}

@test "upload hmm" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    daemon_kill
}

@test "download hmm" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm

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

@test "get non-existent hmm" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me

    run send "hmm_get_by_id 743982 | {1}"
    assert_output 'fail'

    daemon_kill
}

@test "get hmm" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    local -r answer='ok {"id":1,"xxh3":-7843725841264658444,"filename":"PF02545.hmm","job_id":1}'
    run send "hmm_get_by_id 1 | {1} {2}"
    assert_output "$answer"

    run send "hmm_get_by_xxh3 -7843725841264658444 | {1} {2}"
    assert_output "$answer"

    run send "hmm_get_by_job_id 1 | {1} {2}"
    assert_output "$answer"

    run send "hmm_get_by_filename PF02545.hmm | {1} {2}"
    assert_output "$answer"

    daemon_kill
}

@test "upload db without hmm" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.dcp

    run send "wipe | {1}"
    assert_output "ok"

    run send "db_up PF02545.dcp | {1}"
    assert_output "fail"

    daemon_kill
}

@test "upload db" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "db_up PF02545.dcp | {1}"
    assert_output "ok"

    daemon_kill
}

@test "download db" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "db_up PF02545.dcp | {1}"
    assert_output "ok"

    run sendw 0.5 "db_dl 2407434389743836934 output.hmm"
    assert_file_exists output.hmm

    run checksum output.hmm
    assert_output 62f3961caa6580baf68126947031af16cff90ae7f6ec0a0ec0f7b2d7950da8e1

    daemon_kill
}

@test "get non-existent db" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me

    run send "db_get_by_id 743982 | {1}"
    assert_output 'fail'

    daemon_kill
}

@test "get db" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "db_up PF02545.dcp | {1}"
    assert_output "ok"

    local -r answer='ok {"id":1,"xxh3":2407434389743836934,"filename":"PF02545.dcp","hmm_id":1}'
    run send "db_get_by_id 1 | {1} {2}"
    assert_output "$answer"

    run send "db_get_by_xxh3 2407434389743836934 | {1} {2}"
    assert_output "$answer"

    run send "db_get_by_hmm_id 1 | {1} {2}"
    assert_output "$answer"

    run send "db_get_by_filename PF02545.dcp | {1} {2}"
    assert_output "$answer"

    daemon_kill
}

@test "get pending press job" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "job_next_pend | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    daemon_kill
}

@test "set job state" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "job_set_state 1 run | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"run","progress":0,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":0\}'

    run send "job_inc_progress 1 5 | {1}"
    assert_output "ok"

    run send "job_set_state 1 done | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"done","progress":5,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":-?[0-9]+\}'

    daemon_kill
}

@test "submit scan job" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp
    download consensus.faa

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "db_up PF02545.dcp | {1}"
    assert_output "ok"

    run send "job_set_state 1 done | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"done","progress":0,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":-?[0-9]+\}'

    run send "scan_submit 1 1 0 consensus.faa | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    daemon_kill
}

@test "get pending scan job" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp
    download consensus.faa

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "db_up PF02545.dcp | {1}"
    assert_output "ok"

    run send "job_set_state 1 done | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"done","progress":0,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":-?[0-9]+\}'

    run send "scan_submit 1 1 0 consensus.faa | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    run send "job_next_pend | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    daemon_kill
}

@test "get scan job" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp
    download consensus.faa

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "db_up PF02545.dcp | {1}"
    assert_output "ok"

    run send "job_set_state 1 done | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"done","progress":0,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":-?[0-9]+\}'

    run send "scan_submit 1 1 0 consensus.faa | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    run send "job_next_pend | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    run send "scan_get_by_job_id 2 | {1} {2}"
    assert_output 'ok {"id":1,"db_id":1,"multi_hits":true,"hmmer3_compat":false,"job_id":2}'

    daemon_kill
}

@test "count scan job sequences" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp
    download consensus.faa

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "db_up PF02545.dcp | {1}"
    assert_output "ok"

    run send "job_set_state 1 done | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"done","progress":0,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":-?[0-9]+\}'

    run send "scan_submit 1 1 0 consensus.faa | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    run send "job_next_pend | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    run send "scan_get_by_job_id 2 | {1} {2}"
    assert_output 'ok {"id":1,"db_id":1,"multi_hits":true,"hmmer3_compat":false,"job_id":2}'

    run send "scan_seq_count 1 | {1} {2}"
    assert_output 'ok 3'

    daemon_kill
}

@test "download scan job sequences" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp
    download consensus.faa

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "db_up PF02545.dcp | {1}"
    assert_output "ok"

    run send "job_set_state 1 done | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"done","progress":0,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":-?[0-9]+\}'

    run send "scan_submit 1 1 0 consensus.faa | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    run send "job_next_pend | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    run send "scan_get_by_job_id 2 | {1} {2}"
    assert_output 'ok {"id":1,"db_id":1,"multi_hits":true,"hmmer3_compat":false,"job_id":2}'

    run send "scan_dl_seqs 1 seqs.json | {1}"
    assert_output 'ok'
    assert_file_exists seqs.json

    run checksum seqs.json
    assert_output 12ac6942c48d95d34544e07262d30c62d838e7caa93dc4f3fe4e9dd2a2935c0d

    daemon_kill
}

@test "upload scan job product" {
    daemon_spawn schedy -u http://127.0.0.1:49329 -k change-me
    download PF02545.hmm
    download PF02545.dcp
    download consensus.faa
    download prods_file_20221021.tsv

    run send "wipe | {1}"
    assert_output "ok"

    run send "hmm_up PF02545.hmm | {1}"
    assert_output "ok"

    run send "db_up PF02545.dcp | {1}"
    assert_output "ok"

    run send "job_set_state 1 done | {1} {2}"
    assert_output -e 'ok \{"id":1,"type":1,"state":"done","progress":0,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":-?[0-9]+\}'

    run send "scan_submit 1 1 0 consensus.faa | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    run send "job_next_pend | {1} {2}"
    assert_output -e 'ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'

    run send "scan_get_by_job_id 2 | {1} {2}"
    assert_output 'ok {"id":1,"db_id":1,"multi_hits":true,"hmmer3_compat":false,"job_id":2}'

    run send "scan_dl_seqs 1 seqs.json | {1}"
    assert_output 'ok'
    assert_file_exists seqs.json

    run checksum seqs.json
    assert_output 12ac6942c48d95d34544e07262d30c62d838e7caa93dc4f3fe4e9dd2a2935c0d

    run send "prods_file_up prods_file_20221021.tsv | {1}"
    assert_output ok

    daemon_kill
}
