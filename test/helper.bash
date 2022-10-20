#!/bin/bash

checksum() {
    local -r file=$1
    local cmd=""
    if which sha256sum >/dev/null; then
        cmd="sha256sum"
    elif which shasum >/dev/null; then
        cmd="shasum -a 256"
    else
        exit 1
    fi
    $cmd "$file" | cut -d' ' -f1
}

download() {
    local -r file=$1
    local -r hash=$2
    pooch https://pub.danilohorta.me/deciphon/"$file" --hash "$hash"
}

recv() {
    local cnt=50
    local output=
    while [ $cnt -gt 0 ]; do
        output=$(timeout -t 100 cat "$1")
        if [ -n "$output" ]; then
            break
        fi
        cnt=$((cnt - 1))
    done
    echo "$output"
}

send() {
    echo "$1" >stdin
    recv stdout
}

send_nb() {
    echo "$1" >stdin
}
