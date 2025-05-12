#!/bin/bash
set -e

/project/lib/shared_setup.sh meta_s

cd /project/scripts
make -f Makefile_cache || true

chown meta_s:meta_s -R /project/objects

echo "[+] All done!"

/usr/sbin/sshd -D