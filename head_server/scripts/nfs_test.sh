LOG_FILE="./logs/nfs_test.log"
echo -n "" > $LOG_FILE

reset_mount() {
    umount mount
    mount -t nfs 10.105.137.51:/srv/nfs/shared mount
}

perform_test() {
    echo "### Test $1 ###" | tee -a $LOG_FILE
    reset_mount
    ./auto_test.sh $LOG_FILE w # MT and ST write tests
    reset_mount
    ./auto_test.sh $LOG_FILE r st # Single-threaded read test
    reset_mount
    ./auto_test.sh $LOG_FILE r mt # Multi-threaded read test
}


for i in {1..5}; do
    perform_test $i
done