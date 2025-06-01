if [[ -z $1 ]]; then
    echo "Usage: $0 log_file w|r|rw [mt|st]"
    echo "w: Write test only"
    echo "r: Read test only"
    echo "rw: Read and Write tests"
    echo "mt: Multi-threaded copy"
    echo "st: Single-threaded copy"
    exit 1
fi


# Paths
MOUNT_DIR="./mount"
COPY_MT_BIN="./bin/cp"
COPY_MT_SRC="./copy_mt.c"
READ_DIR="./mount_read"
LOG_FILE=$1

# Files to test
FILES=("1mb.bin" "10mb.bin" "50mb.bin" "100mb.bin" "200mb.bin")

# Make sure bin dir exists
mkdir -p ./bin

# Compile multithreaded copy binary
# echo "Compiling multi-threaded copy binary..."
gcc -o "$COPY_MT_BIN" "$COPY_MT_SRC" -lpthread || { echo "Compilation failed"; exit 1; }

mylog() {
    echo $1 | tee -a "$LOG_FILE"
}

# Helper function to run a timed cp command
run_test() {
    local desc=$1
    local cmd=$2
    # echo "$desc... command: $cmd"
    START=$(date +%s.%N)
    eval "$cmd"
    END=$(date +%s.%N)
    ELAPSED=$(echo "scale=6; $END - $START" | bc)
    # printf "%s completed in %.2f seconds\n\n" "$desc" "$ELAPSED"
    printf "%.3f\n" "$ELAPSED" | tee -a "$LOG_FILE"
}

# echo "======================"
# echo "Starting NFS I/O Benchmark"
# echo "======================"

if [[ $2 =~ "w" || $2 =~ "rw" ]]; then
    if [[ -z $3 || $3 =~ "st" ]]; then
        # --- Write ST ---
        mylog "Write ST"
        for file in "${FILES[@]}"; do
            run_test "Write ST: $file" "cp $file $MOUNT_DIR/$file"
        done
    fi

    if [[ -z $3 || $3 =~ "mt" ]]; then
        # --- Write MT ---
        mylog "Write MT"
        for file in "${FILES[@]}"; do
            run_test "Write MT: $file" "$COPY_MT_BIN $file $MOUNT_DIR/$file"
        done
    fi
fi


if [[ $2 =~ "r" || $2 =~ "rw" ]]; then
    if [[ -z $3 || $3 =~ "st" ]]; then
        # --- Read ST ---
        mylog "Read ST" 
        for file in "${FILES[@]}"; do
            run_test "Read ST: $file" "cp $MOUNT_DIR/$file $READ_DIR/$file"
        done
    fi

    if [[ -z $3 || $3 =~ "mt" ]]; then
        # --- Read MT ---
        mylog "Read MT"
        for file in "${FILES[@]}"; do
            run_test "Read MT: $file" "$COPY_MT_BIN $MOUNT_DIR/$file $READ_DIR/$file"
        done
    fi
fi

echo "Benchmark complete!"