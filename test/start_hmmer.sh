#!/bin/sh
set -e

test -e "$1.h3f" || pipx run h3daemon press "$1"
pipx run h3daemon start "$1" --force --port 51371
