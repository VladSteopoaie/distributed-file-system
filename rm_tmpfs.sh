#!/bin/bash

# docker exec head_server /project/lib/rm_tmpfs.sh
# docker exec storage_server1 /project/lib/rm_tmpfs.sh
# docker exec storage_server2 /project/lib/rm_tmpfs.sh
# docker exec storage_server3 /project/lib/rm_tmpfs.sh

ssh head_server /project/lib/rm_tmpfs.sh head_s
ssh storage_s1 /project/lib/rm_tmpfs.sh storage_s1
ssh storage_s2 /project/lib/rm_tmpfs.sh storage_s2
ssh storage_s3 /project/lib/rm_tmpfs.sh storage_s3