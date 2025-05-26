#!/bin/bash
set -e

/project/lib/shared_setup.sh head_s

cd /project/lib/
rm -rf metadata.pb* || true
protoc --cpp_out=. metadata.proto
mv metadata.pb.cc metadata.pb.cpp

cd /project/scripts
make -f Makefile_fs || true
make -f Makefile_storage_mngr || true

chown head_s:head_s -R /project/objects
chown head_s:head_s -R /project/scripts

echo "[+] All done!"

# /usr/sbin/sshd -D