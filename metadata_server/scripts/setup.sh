#!/bin/bash
set -e

/project/lib/shared_setup.sh meta_s

cd /project/lib/
rm -rf metadata.pb* || true
protoc --cpp_out=. metadata.proto
mv metadata.pb.cc metadata.pb.cpp

cd /project/scripts
make -f Makefile_cache || true

chown meta_s:meta_s -R /project/objects

echo "[+] All done!"

# /usr/sbin/sshd -D