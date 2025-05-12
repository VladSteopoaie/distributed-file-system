#!/bin/bash
set -e

/project/lib/shared_setup.sh head_s

cd /project/scripts
make -f Makefile_fs || true
make -f Makefile_storage_mngr || true

chown head_s:head_s -R /project/objects

echo "[+] All done!"

/usr/sbin/sshd -D