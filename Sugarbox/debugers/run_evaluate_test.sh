#!/bin/bash
# Automated evaluate test:
#   1. Launches Sugarbox in headless mode with debug server
#   2. Waits for the server to be ready
#   3. Runs test_evaluate.py
#   4. Kills Sugarbox and reports result

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BINARY="$REPO_ROOT/build/Sugarbox/Sugarbox"
TEST_SCRIPT="$SCRIPT_DIR/test_evaluate.py"
PORT=1234
TIMEOUT=10  # seconds to wait for server

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Sugarbox binary not found at $BINARY"
    echo "Run: cmake --build build"
    exit 1
fi

# Launch Sugarbox in background: headless, break on start, debug server on $PORT
echo "Launching Sugarbox (headless, debug server on port $PORT)..."
"$BINARY" --hide -d --ds "$PORT" &
SUGARBOX_PID=$!

cleanup() {
    echo ""
    echo "Stopping Sugarbox (PID $SUGARBOX_PID)..."
    kill "$SUGARBOX_PID" 2>/dev/null || true
    wait "$SUGARBOX_PID" 2>/dev/null || true
}
trap cleanup EXIT

# Wait until port is open
echo -n "Waiting for debug server..."
ELAPSED=0
while ! nc -z 127.0.0.1 "$PORT" 2>/dev/null; do
    sleep 0.5
    ELAPSED=$(echo "$ELAPSED + 0.5" | bc)
    if (( $(echo "$ELAPSED >= $TIMEOUT" | bc -l) )); then
        echo ""
        echo "ERROR: Debug server did not start within ${TIMEOUT}s"
        exit 1
    fi
    echo -n "."
done
echo " ready."
echo ""

# Run tests
python3 "$TEST_SCRIPT" --port "$PORT"
TEST_EXIT=$?

exit $TEST_EXIT
