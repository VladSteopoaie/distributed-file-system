#!/bin/bash

if [[ " $@ " =~ " docker " ]]; then
    # Add Docker's official GPG key:
    sudo apt-get update
    sudo apt-get install ca-certificates curl
    sudo install -m 0755 -d /etc/apt/keyrings
    sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
    sudo chmod a+r /etc/apt/keyrings/docker.asc

    # Add the repository to Apt sources:
    echo \
    "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
    $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | \
    sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
    sudo apt-get update

    sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
fi

if [[ " $@ " =~ " shared " ]]; then
    docker rmi -f shared_image
    docker build -t shared_image -f ../dockerfile_shared ..
fi

docker rmi -f storage_server
docker build -t storage_server -f dockerfile ..
read -p "Enter username: " username
docker run --rm --net host -it \
    --cap-add SYS_ADMIN \
    --cap-add NET_ADMIN \
    --cap-add NET_RAW \
    --privileged \
    --cpuset-cpus 0-7 \
    --cpus="8.0" \
    storage_server bash -c "/project/scripts/setup.sh $username"