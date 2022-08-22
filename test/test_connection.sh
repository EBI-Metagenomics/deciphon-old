#!/bin/bash

unittest_origin=""
source "$PWD/unittest.sh"

unittest_run schedy
unittest_sendnl "CONNECT http://100.71.124.99:49329 change-me"
unittest_sendnl "ONLINE"

echo "OK" >"$unittest_origin/desired"
echo "YES" >>"$unittest_origin/desired"

sleep 0.3
diff "$unittest_origin/desired" "$(unittest_stdout)"
