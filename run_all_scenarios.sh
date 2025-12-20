#!/bin/bash

SERVER_HOST="127.0.0.1"
SERVER_PORT="8080"
BASE_URL="https://${SERVER_HOST}:${SERVER_PORT}"

# =============================
#  LOG DIRECTORY
# =============================
LOG_DIR="scripts/benchmark_logs"
mkdir -p $LOG_DIR

# =============================
#  SCENARIOS DEFINITIONS
# =============================

declare -a SCENARIOS=(
    # 1 - Light Load
    "wrk -t2 -c10 -d30s ${BASE_URL}/"

    # 2 - Static file load
    "wrk -t4 -c50 -d30s ${BASE_URL}/index.html"

    # 3 - Medium GET
    "wrk -t8 -c200 -d30s ${BASE_URL}/api/test"

    # 4 - Mixed routes using routes.lua
    "wrk -t8 -c200 -d30s -s ./scripts/routes.lua ${BASE_URL}/"

    # 5 - POST workload
    "wrk -t10 -c300 -d30s -s ./scripts/post.lua ${BASE_URL}/api/post"

    # 6 - Heavy load (CPU high)
    "wrk -t20 -c400 -d45s ${BASE_URL}/api/heavy"

    # 7 - Extreme load (queue explosion)
    "wrk -t32 -c800 -d30s ${BASE_URL}/"

    # 8 - Burst traffic
    "wrk -t4 -c800 -d10s ${BASE_URL}/"

    # 9 - Long duration test
    "wrk -t16 -c300 -d120s ${BASE_URL}/"

    # 10 - Request size variation
    "wrk -t10 -c200 -d30s -s ./scripts/body_size.lua ${BASE_URL}/api/post"
)

# =============================
#  RUN ALL SCENARIOS FOR 1 ALGO
# =============================
run_phase() {
    local ALGO=$1
    local PHASE_DIR="${LOG_DIR}/${ALGO}"
    mkdir -p ${PHASE_DIR}

    echo ""
    echo "====================================="
    echo "   🔥 STARTING PHASE: ${ALGO}"
    echo "====================================="
    
    # Start server with algorithm
    echo "🚀 Starting server with algorithm: ${ALGO}"
    ./build/server/http_server --algo=${ALGO} &
    SERVER_PID=$!
    sleep 2

    # Run 10 scenarios
    for i in "${!SCENARIOS[@]}"; do
        SC_INDEX=$((i+1))
        TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
        OUT_FILE="${PHASE_DIR}/scenario_${SC_INDEX}_${TIMESTAMP}.log"

        echo ""
        echo "-------------------------------------"
        echo "▶ Running Scenario ${SC_INDEX}"
        echo "Command: ${SCENARIOS[$i]}"
        echo "Saving log → ${OUT_FILE}"
        
        bash -c "${SCENARIOS[$i]}" > ${OUT_FILE}
        echo "✔ Done."
    done

    # Stop server
    echo ""
    echo "🛑 Stopping server (PID=${SERVER_PID})"
    kill -9 ${SERVER_PID}
    sleep 2
}

# =============================
#  RUN 4 PHASES
# =============================
PHASES=("FIFO" "SJF" "RR" "WFQ")

for algo in "${PHASES[@]}"; do
    run_phase ${algo}
done

echo ""
echo "====================================="
echo "🎉 DONE! All 40 WRK scenarios finished."
echo "Logs are stored in: ${LOG_DIR}"
echo "====================================="
