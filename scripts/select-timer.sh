#!/bin/sh

DIR=$(realpath "$(dirname "$0")")

while true; do
  "$DIR/select.sh"
  sleep 1
done
