#!/bin/bash
pkill -f "http.server 8080" 2>/dev/null
sleep 1
pkill proxy 2>/dev/null
sleep 1
# verify it's free
if ss -tlnp | grep -q :8080; then
    echo "FAIL: port 8080 still in use"
    exit 1
fi
set -e

# Build
make clean && make

# Start backend
python3 -m http.server 8080 --directory www &
BACKEND_PID=$!
sleep 1

# Start proxy
./proxy &
PROXY_PID=$!
sleep 1

# Test 1: valid route
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -H "Host: alice.eezydep.org" http://localhost:8000/)
[ "$RESPONSE" = "200" ] && echo "PASS: valid route" || echo "FAIL: valid route (got $RESPONSE)"

# Test 2: unknown host
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -H "Host: unknown.eezydep.org" http://localhost:8000/)
[ "$RESPONSE" = "502" ] && echo "PASS: unknown host" || echo "FAIL: unknown host (got $RESPONSE)"

# Test 3: health check (kill backend, wait, curl)
kill $BACKEND_PID
sleep 35
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -H "Host: alice.eezydep.org" http://localhost:8000/)
[ "$RESPONSE" = "503" ] && echo "PASS: unhealthy backend" || echo "FAIL: unhealthy backend (got $RESPONSE)"

# Cleanup
kill $PROXY_PID 2>/dev/null
echo "Done"