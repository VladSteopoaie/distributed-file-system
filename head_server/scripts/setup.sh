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

fallocate -l 1M 1mb.bin
fallocate -l 10M 10mb.bin
fallocate -l 100M 100mb.bin
fallocate -l 50M 50mb.bin
fallocate -l 200M 200mb.bin

echo "[+] All done!"

# /usr/sbin/sshd -D