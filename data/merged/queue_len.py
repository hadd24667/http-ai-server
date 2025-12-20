import pandas as pd
import numpy as np

# ===============================
# 1. Load log CSV
# ===============================
file = "dataset_training.csv"   # đổi tên theo file của bạn
df = pd.read_csv(file)

print("===== BASIC INFO =====")
print("Rows:", len(df))
print("Columns:", df.columns.tolist())

# ===============================
# 2. Tính queue_len trung bình
# ===============================
mean_q = df["queue_len"].mean()
median_q = df["queue_len"].median()
min_q = df["queue_len"].min()
max_q = df["queue_len"].max()

print("\n===== QUEUE LENGTH SUMMARY =====")
print(f"Average queue_len : {mean_q:.3f}")
print(f"Median queue_len  : {median_q}")
print(f"Min queue_len     : {min_q}")
print(f"Max queue_len     : {max_q}")

# ===============================
# 3. Phân phối queue_len (histogram)
# ===============================
print("\n===== QUEUE LEN DISTRIBUTION =====")
dist = df["queue_len"].value_counts().sort_index()
print(dist)

# ===============================
# 4. Bin queue_len thành nhóm
# ===============================
bins = [0, 1, 5, 10, 20, 50, 100, np.inf]
labels = ["0-1", "1-5", "5-10", "10-20", "20-50", "50-100", ">100"]

df["queue_bin"] = pd.cut(df["queue_len"], bins=bins, labels=labels, right=False)

print("\n===== QUEUE BIN DISTRIBUTION =====")
print(df["queue_bin"].value_counts().sort_index())
