LOG_FILE="./logs/dfs_test.log"
echo -n "" > $LOG_FILE

perform_test() {
    echo "### Test $1 ###" | tee -a $LOG_FILE
    ./auto_test.sh $LOG_FILE wr
}

for i in {1..5}; do
    perform_test $i
done