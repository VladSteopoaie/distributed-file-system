#!/bin/bash

mkdir -p /mnt/tmpfs
mount -t tmpfs -o size=1M tmpfs /mnt/tmpfs
chown -R $1:$1 /mnt/tmpfs
df -h /mnt/tmpfs 