#!/bin/sh
set -e

pipx run h3daemon start "$1" --port 51371 --force
pipx run h3daemon isready "$1" --wait
