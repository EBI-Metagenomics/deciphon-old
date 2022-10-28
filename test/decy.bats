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
    download PF02545.dcp
    download consensus.fna
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
