#!/bin/bash

docker exec head_server /project/lib/rm_tmpfs.sh
docker exec storage_server1 /project/lib/rm_tmpfs.sh
docker exec storage_server2 /project/lib/rm_tmpfs.sh
docker exec storage_server3 /project/lib/rm_tmpfs.sh
