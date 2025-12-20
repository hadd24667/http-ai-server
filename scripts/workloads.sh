#!/bin/bash

# Base URL (HTTPS)
SERVER="https://127.0.0.1:8080"

# W1–W3: base workload (nhẹ) → chỉ để fill data, KHÔNG dùng chọn best_algo
# W4–W10: heavy + queue bomb → dùng để chọn best_algo

WORKLOADS=(
  # W1 - Light homepage
  "wrk -t2  -c20   -d20s ${SERVER}/"

  # W2 - Static index.html
  "wrk -t4  -c40   -d20s ${SERVER}/index.html"

  # W3 - Medium read-only
  "wrk -t4  -c60   -d25s ${SERVER}/api/test"

  # W4 - Mixed routes (bắt đầu nặng hơn)
  "wrk -t8  -c150  -d30s -s ./scripts/routes.lua ${SERVER}/"

  # W5 - POST workload
  "wrk -t8  -c200  -d30s -s ./scripts/post.lua ${SERVER}/api/post"

  # W6 - CPU heavy
  "wrk -t12 -c260  -d35s ${SERVER}/api/heavy"

  # W7 - Queue bomb 1 (concurrency rất lớn)
  "wrk -t8  -c600  -d25s ${SERVER}/"

  # W8 - Queue bomb 2 (heavy API)
  "wrk -t8  -c800  -d25s ${SERVER}/api/heavy"

  # W9 - Long heavy duration
  "wrk -t12 -c400  -d40s ${SERVER}/"

  # W10 - Body size variation + high load
  "wrk -t10 -c500  -d30s -s ./scripts/body_size.lua ${SERVER}/api/post"
)
