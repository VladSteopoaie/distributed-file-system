#!/bin/bash

docker exec head_server cp /mnt/tmpfs/fs.log /project/scripts/logs/
docker exec head_server cp /mnt/tmpfs/storage_mngr.log /project/scripts/logs/
docker exec head_server cp /mnt/tmpfs/s_client.log /project/scripts/logs/
docker exec storage_server1 cp /mnt/tmpfs/storage1.log /project/scripts/logs/
docker exec storage_server2 cp /mnt/tmpfs/storage2.log /project/scripts/logs/
docker exec storage_server3 cp /mnt/tmpfs/storage3.log /project/scripts/logs/
