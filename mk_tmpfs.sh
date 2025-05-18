#!/bin/bash

docker exec head_server /project/lib/mk_tmpfs.sh head_s
docker exec storage_server1 /project/lib/mk_tmpfs.sh storage_s1
docker exec storage_server2 /project/lib/mk_tmpfs.sh storage_s2
docker exec storage_server3 /project/lib/mk_tmpfs.sh storage_s3