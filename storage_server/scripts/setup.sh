#!/bin/bash
set -x

/project/lib/shared_setup.sh $1

mkdir -p /project/storage
mkdir -p /project/scripts/bin

# cd /project/lib/
# rm -rf metadata.pb* || true
# protoc --cpp_out=. metadata.proto
# mv metadata.pb.cc metadata.pb.cpp

cd /project/scripts
make -f Makefile_storage || true

chown $1:$1 -R /project/objects
chown $1:$1 -R /project/storage
# chown $1:$1 -R /project/scripts

echo "[+] Done setting up protobuf"

echo "[+] All done!"
/usr/sbin/sshd -D

# sleep infinity