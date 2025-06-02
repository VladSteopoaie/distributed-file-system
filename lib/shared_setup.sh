#!/bin/bash
set -e

USERNAME="$1"
USER_UID="${2:-1001}"           # Default to 1001 if not given
USER_GID="$USER_UID"

if [ -z "$USERNAME" ]; then
    echo "Usage: $0 <username> [uid]"
    exit 1
fi

mkdir -p /etc/sudoers.d/

echo "[+] Creating group $USERNAME with GID $USER_GID"
groupadd --gid "$USER_GID" "$USERNAME"

echo "[+] Creating user $USERNAME with UID $USER_UID"
useradd --uid "$USER_UID" --gid "$USER_GID" -m -s /bin/bash "$USERNAME"

echo "[+] Setting up passwordless sudo for $USERNAME"
echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" > "/etc/sudoers.d/$USERNAME"
chmod 0440 "/etc/sudoers.d/$USERNAME"

echo "[+] Setting up SSH directory"
SSH_DIR="/home/$USERNAME/.ssh"
SSH_ROOT_DIR="/root/.ssh"
mkdir -p "$SSH_DIR"
mkdir -p "$SSH_ROOT_DIR"
chmod 700 "$SSH_DIR"

echo "[+] Copying SSH config and keys"
cp /project/mpi/ssh_config "$SSH_DIR/config"
cp /project/mpi/id_rsa "$SSH_DIR"
cp /project/mpi/id_rsa.pub "$SSH_DIR"
cat "$SSH_DIR/id_rsa.pub" >> "$SSH_DIR/authorized_keys"

echo "[+] Setting ownership and permissions"
chown -R "$USERNAME:$USERNAME" "$SSH_DIR"
chmod 600 "$SSH_DIR/id_rsa"
chmod 600 "$SSH_DIR/authorized_keys"
chown -R $USERNAME:$USERNAME /home/$USERNAME
cp "$SSH_DIR"/* "$SSH_ROOT_DIR"
# chown -R $USERNAME:$USERNAME /project/scripts

echo "[+] Done setting up user $USERNAME"

echo "export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH" >> /home/$USERNAME/.bashrc
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# cd /project/lib/
# rm -rf metadata.pb*
# protoc --cpp_out=. metadata.proto
# mv metadata.pb.cc metadata.pb.cpp
# echo "[+] Done setting up protobuf"
