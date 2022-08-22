#!/bin/bash

program=$1
stdin=$2
stdout=$3
stderr=$4

tail -f "$stdin" | "$program" >"$stdout" 2>"$stderr"
