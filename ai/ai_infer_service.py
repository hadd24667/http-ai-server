import os
from typing import Optional, Dict, Any

import joblib
import pandas as pd
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

MODEL_PATH = os.getenv("MODEL_PATH", os.path.join(BASE_DIR, "models", "xgb_scheduler_v2.joblib"))
ENCODER_PATH = os.getenv("ENCODER_PATH", os.path.join(BASE_DIR, "models", "label_encoder_scheduler_v2.joblib"))

# Load sklearn Pipeline: preprocess(ColumnTransformer) + clf(XGBClassifier)
try:
    model = joblib.load(MODEL_PATH)
except Exception as e:
    raise RuntimeError(f"Cannot load model: {MODEL_PATH} -> {e}")

label_encoder = None
try:
    label_encoder = joblib.load(ENCODER_PATH)
except Exception:
    label_encoder = None


FEATURE_ORDER = [
    "cpu",
    "queue_len",
    "queue_bin",
    "request_method",
    "request_path_length",
    "estimated_workload",
    "req_size",
]

def compute_queue_bin(q: int) -> int:
    # đúng theo bins bạn dùng trong training (0-5, 5-20, 20-50, 50-100, 100-200, 200-400, 400+)
    if q <= 5:   return 0
    if q <= 20:  return 1
    if q <= 50:  return 2
    if q <= 100: return 3
    if q <= 200: return 4
    if q <= 400: return 5
    return 6


class PredictReq(BaseModel):
    cpu: float
    queue_len: int

    # optional: nếu C++ không gửi, server tự tính từ queue_len
    queue_bin: Optional[int] = None

    request_method: str
    request_path_length: int
    estimated_workload: float
    req_size: int

    class Config:
        extra = "ignore"


app = FastAPI(title="Scheduler AI Inference (Notebook-Compatible)", version="1.0")


@app.get("/health")
def health():
    return {
        "status": "ok",
        "model_loaded": True,
        "encoder_loaded": label_encoder is not None,
        "feature_order": FEATURE_ORDER,
    }

@app.post("/predict")
def predict(req: PredictReq):
    data = req.dict()

    if data["queue_bin"] is None:
        data["queue_bin"] = compute_queue_bin(int(data["queue_len"]))

    # FIX KIỂU DỮ LIỆU categorical (QUAN TRỌNG)
    data["request_method"] = str(data["request_method"])
    data["queue_bin"] = str(data["queue_bin"])

    row = {
        "cpu": float(data["cpu"]),
        "queue_len": int(data["queue_len"]),
        "queue_bin": data["queue_bin"],                    # str
        "request_method": data["request_method"],          # str
        "request_path_length": int(data["request_path_length"]),
        "estimated_workload": float(data["estimated_workload"]),
        "req_size": int(data["req_size"]),
    }

    X = pd.DataFrame([row])

    try:
        y = model.predict(X)[0]
        y_int = int(y)

        algo = (
            label_encoder.inverse_transform([y_int])[0]
            if label_encoder is not None
            else str(y_int)
        )

        return {
            "algorithm": algo,
            "raw_pred": y_int,
            "input_used": row
        }

    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Prediction failed: {e}")
