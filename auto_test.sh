# Paths
MOUNT_DIR="./mount"
COPY_MT_BIN="./bin/cp"
COPY_MT_SRC="./copy_mt.c"
READ_DIR="./read_mount"

# Files to test
FILES=("1mb.bin" "10mb.bin" "50mb.bin" "100mb.bin" "200mb.bin")

# Make sure bin dir exists
mkdir -p ./bin

# Compile multithreaded copy binary
echo "🔧 Compiling multi-threaded copy binary..."
gcc -o "$COPY_MT_BIN" "$COPY_MT_SRC" -lpthread || { echo "❌ Compilation failed"; exit 1; }

# Helper function to run a timed cp command
run_test() {
    local desc=$1
    local cmd=$2
    # echo "▶️  $desc... command: $cmd"
    START=$(date +%s.%N)
    eval "$cmd"
    END=$(date +%s.%N)
    ELAPSED=$(echo "scale=6; $END - $START" | bc)
    # printf "✅ %s completed in %.2f seconds\n\n" "$desc" "$ELAPSED"
    printf "%.3f\n" "$ELAPSED"
}

echo "======================"
echo "🚀 Starting NFS I/O Benchmark"
echo "======================"

# --- Write ST ---
echo "📝 Write Test - Single Threaded"
for file in "${FILES[@]}"; do
    run_test "Write ST: $file" "cp $file $MOUNT_DIR/$file"
done

# --- Write MT ---
echo "📝 Write Test - Multi Threaded"
for file in "${FILES[@]}"; do
    run_test "Write MT: $file" "$COPY_MT_BIN $file $MOUNT_DIR/$file"
done

# --- Read ST ---
echo "📖 Read Test - Single Threaded"
for file in "${FILES[@]}"; do
    run_test "Read ST: $file" "cp $MOUNT_DIR/$file $READ_DIR/$file"
done

# --- Read MT ---
echo "📖 Read Test - Multi Threaded"
for file in "${FILES[@]}"; do
    run_test "Read MT: $file" "$COPY_MT_BIN $MOUNT_DIR/$file $READ_DIR/$file"
done

echo "🏁 Benchmark complete!"