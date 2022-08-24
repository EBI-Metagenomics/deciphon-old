#!/bin/bash

prog=$1

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
. "$SCRIPT_DIR/unittest"

unittest_exec "$prog"
unittest_sendnl "CONNECT http://100.71.124.99:49329 change-me"
unittest_sendnl "ONLINE"
unittest_sendnl "WIPE"
unittest_sendnl "HMM_UP /Users/horta/data/minifam.hmm"
unittest_stop

{
    echo "OK"
    echo "YES"
    echo "OK"
    echo "OK"
} >desired

diff stdout desired
