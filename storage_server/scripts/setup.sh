#!/bin/bash
set -e

/project/lib/shared_setup.sh $1

mkdir -p /project/storage
cd /project/scripts
make -f Makefile_storage || true

chown $1:$1 -R /project/objects
chown $1:$1 -R /project/storage

echo "[+] All done!"

/usr/sbin/sshd -D