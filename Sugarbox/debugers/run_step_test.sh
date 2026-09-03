#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BINARY="$REPO_ROOT/build/Sugarbox/Sugarbox"
PORT=1234
TIMEOUT=10

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Sugarbox binary not found at $BINARY"
    exit 1
fi

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

python3 "$SCRIPT_DIR/test_step.py" --port "$PORT"
exit $?
