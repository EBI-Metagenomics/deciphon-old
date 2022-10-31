#!/bin/bash

setup() {
    bats_load_library bats-support
    bats_load_library bats-assert
    bats_load_library bats-file
    load helper
    PATH="$BATS_TEST_DIRNAME/..:$PATH"
    helper_init
    if [ -z ${DCP_API_HOST+x} ]; then
        DCP_API_HOST=127.0.0.1
    fi
    daemon_spawn decy
    download PF02545.hmm
    download Pfam-A.5.hmm
    download Pfam-A.10.hmm
    download query1.fna
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
    run sendo "fwd schedy online | echo {1}"
    assert_output "echo yes"
}

@test "upload hmm" {
    send "fwd schedy wipe"
    run sendo "fwd schedy hmm_up PF02545.hmm | echo {1}"
    assert_output "echo ok"
}

@test "get hmm" {
    send "fwd schedy wipe"
    send "fwd schedy hmm_up PF02545.hmm"
    run sendo "fwd schedy hmm_get_by_filename PF02545.hmm | echo {1} {2}"
    assert_output 'echo ok {"id":1,"xxh3":-7843725841264658444,"filename":"PF02545.hmm","job_id":1}'
}

@test "get press job" {
    send "fwd schedy wipe"
    send "fwd schedy hmm_up PF02545.hmm"
    run sendo "fwd schedy job_get_by_id 1 | echo {1} {2}"
    assert_output -e 'echo ok \{"id":1,"type":1,"state":"(pend|run|done)","progress":[0-9]+,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":[0-9]+\}'
}

@test "submit scan" {
    send "fwd schedy wipe"
    send "fwd schedy hmm_up PF02545.hmm"
    wait_file PF02545.dcp
    run sendo "fwd schedy scan_submit 1 1 0 query1.fna | echo {1} {2}"
    assert_output -e 'echo ok \{"id":2,"type":0,"state":"pend","progress":0,"error":"","submission":[0-9]+,"exec_started":0,"exec_ended":0\}'
}

@test "scan" {
    send "fwd schedy wipe"
    send "fwd schedy hmm_up PF02545.hmm"
    wait_file PF02545.dcp
    send "fwd schedy scan_submit 1 1 0 query1.fna"
    sleep 5
    run sendo "fwd schedy job_get_by_id 2 | echo {1} {2}"
    assert_output -e 'echo ok \{"id":2,"type":0,"state":"done","progress":100,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":[0-9]+\}'
}

@test "multiple press jobs" {
    send "fwd schedy wipe"
    send "fwd schedy hmm_up PF02545.hmm"
    send "fwd schedy hmm_up Pfam-A.10.hmm"
    send "fwd schedy hmm_up Pfam-A.5.hmm"

    wait_file PF02545.dcp
    run sendo "fwd schedy job_get_by_id 1 | echo {1} {2}"
    assert_output -e 'echo ok \{"id":1,"type":1,"state":"done","progress":100,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":[0-9]+\}'

    wait_file Pfam-A.10.dcp
    run sendo "fwd schedy job_get_by_id 2 | echo {1} {2}"
    assert_output -e 'echo ok \{"id":2,"type":1,"state":"done","progress":100,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":[0-9]+\}'

    wait_file Pfam-A.5.dcp
    run sendo "fwd schedy job_get_by_id 3 | echo {1} {2}"
    assert_output -e 'echo ok \{"id":3,"type":1,"state":"done","progress":100,"error":"","submission":[0-9]+,"exec_started":[0-9]+,"exec_ended":[0-9]+\}'

    run file_size PF02545.dcp
    assert_output 1937824

    run file_size Pfam-A.10.dcp
    assert_output 21876575

    run file_size Pfam-A.5.dcp
    assert_output 12397034
}
