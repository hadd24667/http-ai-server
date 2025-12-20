#!/bin/bash
set -e

source ./scripts/workloads.sh

RAW_DIR="data/raw"
SERVER_LOG="data/logs/http_server_log.csv"
mkdir -p "${RAW_DIR}"

for i in "${!WORKLOADS[@]}"; do
  IDX=$((i+1))
  OUT_FILE="${RAW_DIR}/W${IDX}_FIFO.csv"

  echo ""
  echo "======================================="
  echo "[FIFO] Workload W${IDX}"
  echo "======================================="

  # Start server với FIFO & ít thread để dễ tạo queue
  ./build/server/http_server --algo=FIFO --threads=2 &
  SERVER_PID=$!
  sleep 3

  # reset log server
  : > "${SERVER_LOG}"

  echo "[FIFO] CMD: ${WORKLOADS[$i]}"
  bash -c "${WORKLOADS[$i]}"

  # đợi flush log
  sleep 1

  if [ -s "${SERVER_LOG}" ]; then
    cp "${SERVER_LOG}" "${OUT_FILE}"
    echo "[FIFO] Saved -> ${OUT_FILE}"
  else
    echo "[FIFO] ⚠ NO LOG for W${IDX}"
  fi

  kill -9 "${SERVER_PID}" >/dev/null 2>&1 || true
  sleep 1
done
