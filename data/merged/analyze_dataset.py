import pandas as pd
import numpy as np
import sys

# =======================
#   HELPERS
# =======================

QUEUE_BINS = [
    (0, 1),
    (1, 5),
    (5, 10),
    (10, 20),
    (20, 50),
    (50, 100),
    (100, np.inf),
]

def bin_queue_len(x):
    for low, high in QUEUE_BINS:
        if low <= x < high:
            return f"{low}-{int(high) if np.isfinite(high) else 'inf'}"
    return "unknown"


# =======================
#   MAIN ANALYSIS
# =======================
def analyze_dataset(filepath):
    print("\n================= LOADING DATASET =================")
    df = pd.read_csv(filepath)
    print(f"Loaded rows: {len(df)}")
    print(f"Columns: {list(df.columns)}")

    # --------------------------------------------------
    # BASIC queue_len statistics
    # --------------------------------------------------
    print("\n================= QUEUE_LEN SUMMARY =================")
    print(df["queue_len"].describe())

    avg = df["queue_len"].mean()
    med = df["queue_len"].median()
    mn = df["queue_len"].min()
    mx = df["queue_len"].max()

    print(f"\nAverage queue_len : {avg:.3f}")
    print(f"Median queue_len  : {med}")
    print(f"Min queue_len     : {mn}")
    print(f"Max queue_len     : {mx}")

    # --------------------------------------------------
    # QUEUE_LEN distribution
    # --------------------------------------------------
    print("\n================= QUEUE_LEN VALUE COUNTS =================")
    print(df["queue_len"].value_counts().sort_index())

    # --------------------------------------------------
    # QUEUE BINS
    # --------------------------------------------------
    print("\n================= QUEUE_LEN BIN DISTRIBUTION =================")
    df["queue_bin"] = df["queue_len"].apply(bin_queue_len)
    print(df["queue_bin"].value_counts())

    # --------------------------------------------------
    # CLASS DISTRIBUTION (best_algo)
    # --------------------------------------------------
    if "best_algo" in df.columns:
        print("\n================= BEST_ALGO CLASS DISTRIBUTION =================")
        print(df["best_algo"].value_counts())

    # --------------------------------------------------
    # CHECK queue_len distribution for each class
    # --------------------------------------------------
    if "best_algo" in df.columns:
        print("\n================= QUEUE_LEN STATS PER CLASS =================")
        class_stats = df.groupby("best_algo")["queue_len"].describe()
        print(class_stats)

        # Also check bin distribution per class
        print("\n================= QUEUE_LEN BIN DISTRIBUTION PER CLASS =================")
        print(df.groupby("best_algo")["queue_bin"].value_counts())

    # --------------------------------------------------
    # OPTIONAL: correlation matrix
    # --------------------------------------------------
    numeric_cols = df.select_dtypes(include=[np.number])
    if len(numeric_cols.columns) > 1:
        print("\n================= CORRELATION MATRIX =================")
        print(numeric_cols.corr())


# =======================
#   ENTRY POINT
# =======================
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 analyze_dataset.py dataset_training_v2.csv")
        sys.exit(1)

    analyze_dataset(sys.argv[1])
