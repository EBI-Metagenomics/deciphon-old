#!/bin/bash

source "$PWD/unittest.sh"

unittest_exec schedy
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
