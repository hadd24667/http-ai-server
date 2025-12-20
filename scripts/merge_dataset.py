import os
import re
import random
import numpy as np
import pandas as pd

RAW_DIR = "data/raw"
OUT_DIR = "data/merged"
os.makedirs(OUT_DIR, exist_ok=True)

OUT_FILE = os.path.join(OUT_DIR, "dataset_training_v2.csv")

ALGOS = ["FIFO", "SJF", "RR", "WFQ"]

# ----------------------------
# 1. Parse tên file W{idx}_{ALGO}.csv
# ----------------------------
def parse_name(fname: str):
    m = re.match(r"W(\d+)_(\w+)\.csv$", fname)
    if not m:
        return None, None
    w_id = int(m.group(1))
    algo = m.group(2)
    return w_id, algo

files = [f for f in os.listdir(RAW_DIR) if f.endswith(".csv")]

rows = []
for f in files:
    w_id, algo = parse_name(f)
    if w_id is None or algo not in ALGOS:
        continue
    path = os.path.join(RAW_DIR, f)
    df = pd.read_csv(path)

    if "response_ms" not in df.columns or "queue_len" not in df.columns:
        raise ValueError(f"'response_ms' or 'queue_len' not found in {path}")

    df["workload_id"] = w_id
    df["algo_run"] = algo
    rows.append(df)

full = pd.concat(rows, ignore_index=True)
print("Total raw rows (all W, all algos):", len(full))

# ----------------------------
# 2. Chỉ dùng W4–W10 để chọn best_algo_bin
# ----------------------------
full = full[full["workload_id"] >= 4].copy()
print("Rows after keeping W4–W10:", len(full))

# ----------------------------
# 3. Định nghĩa queue_bin
# ----------------------------
def bin_queue_len(q):
    # Bạn có thể chỉnh lại bins cho hợp lý hơn
    if q < 5:   return "0-5"
    if q < 20:  return "5-20"
    if q < 50:  return "20-50"
    if q < 100: return "50-100"
    if q < 200: return "100-200"
    if q < 400: return "200-400"
    return "400+"

full["queue_bin"] = full["queue_len"].apply(bin_queue_len)

# ----------------------------
# 4. Tính avg response_ms cho mỗi (workload_id, queue_bin, algo_run)
# ----------------------------
grp = (
    full.groupby(["workload_id", "queue_bin", "algo_run"])["response_ms"]
    .agg(["count", "mean"])
    .reset_index()
    .rename(columns={"mean": "avg_rt", "count": "n"})
)
print("Grouped rows:", len(grp))

# ----------------------------
# 5. Chọn best_algo_bin cho từng (workload_id, queue_bin)
#    (chỉ chọn nếu mỗi algo có đủ n, hoặc tổng n đủ lớn)
# ----------------------------
MIN_SAMPLES_PER_ALGO = 50  # tuỳ chỉnh

# lọc bỏ những (workload_id, queue_bin, algo_run) quá ít sample
grp_filtered = grp[grp["n"] >= MIN_SAMPLES_PER_ALGO].copy()
print("After filtering by MIN_SAMPLES_PER_ALGO:", len(grp_filtered))

# chọn algo có avg_rt nhỏ nhất trong mỗi (workload_id, queue_bin)
best_rows = grp_filtered.sort_values("avg_rt").groupby(
    ["workload_id", "queue_bin"], as_index=False
).first()

best_rows = best_rows[["workload_id", "queue_bin", "algo_run"]].rename(
    columns={"algo_run": "best_algo_bin"}
)

print("Number of (workload_id, queue_bin) with a best_algo_bin:", len(best_rows))

# ----------------------------
# 6. Join best_algo_bin vào từng dòng
# ----------------------------
full = full.merge(
    best_rows,
    on=["workload_id", "queue_bin"],
    how="left",
)

# bỏ những dòng không xác định được best_algo_bin
full = full[~full["best_algo_bin"].isna()].copy()
print("Rows after dropping rows without best_algo_bin:", len(full))

# ----------------------------
# 7. Cân bằng class theo best_algo_bin (RR/SJF/WFQ, bỏ FIFO nếu muốn)
# ----------------------------
# nếu bạn KHÔNG muốn train FIFO thì bỏ nó
full = full[full["best_algo_bin"].isin(["RR", "SJF", "WFQ"])].copy()

class_counts = full["best_algo_bin"].value_counts()
print("\nClass distribution BEFORE balancing:")
print(class_counts)

min_count = class_counts.min()
parts = []
for algo in class_counts.index:
    part = full[full["best_algo_bin"] == algo]
    if len(part) > min_count:
        part = part.sample(min_count, random_state=42)
    parts.append(part)

balanced = pd.concat(parts, ignore_index=True)
print("\nClass distribution AFTER balancing:")
print(balanced["best_algo_bin"].value_counts())
print("Total balanced rows:", len(balanced))

# ----------------------------
# 8. Chọn subset cột để train
# ----------------------------
keep_cols = [
    "timestamp",
    "cpu",
    "queue_len",
    "queue_bin",
    "request_method",
    "request_path_length",
    "estimated_workload",
    "algo_at_enqueue",
    "algo_at_run",
    "req_size",
    "response_ms",
    "latency_avg",
    "workload_id",
    "algo_run",
    "best_algo_bin",
]
keep_cols = [c for c in keep_cols if c in balanced.columns]

balanced[keep_cols].to_csv(OUT_FILE, index=False)
print("\n✅ Saved training dataset v2 to:", OUT_FILE)
