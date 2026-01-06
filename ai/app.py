from flask import Flask, request, jsonify
import os
import joblib
import pandas as pd

app = Flask(__name__)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Ưu tiên dùng pipeline + encoder giống ai_infer_service.py (khuyên dùng)
MODEL_PATH = os.getenv("MODEL_PATH", os.path.join(BASE_DIR, "models", "xgb_scheduler_v2.joblib"))
ENCODER_PATH = os.getenv("ENCODER_PATH", os.path.join(BASE_DIR, "models", "label_encoder_scheduler_v2.joblib"))

# Fallback nếu bạn chỉ có pkl cũ
FALLBACK_MODEL_PATH = os.getenv("FALLBACK_MODEL_PATH", os.path.join(BASE_DIR, "models", "scheduler_model.pkl"))

def compute_queue_bin(q: int) -> int:
    # bins: 0-5, 5-20, 20-50, 50-100, 100-200, 200-400, 400+
    if q <= 5:   return 0
    if q <= 20:  return 1
    if q <= 50:  return 2
    if q <= 100: return 3
    if q <= 200: return 4
    if q <= 400: return 5
    return 6

# Load model
model = None
label_encoder = None

try:
    model = joblib.load(MODEL_PATH)
except Exception:
    # fallback model cũ
    model = joblib.load(FALLBACK_MODEL_PATH)

try:
    label_encoder = joblib.load(ENCODER_PATH)
except Exception:
    label_encoder = None


@app.get("/health")
def health():
    return jsonify({
        "status": "ok",
        "model_loaded": model is not None,
        "encoder_loaded": label_encoder is not None,
        "model_path": MODEL_PATH if os.path.exists(MODEL_PATH) else FALLBACK_MODEL_PATH
    })


@app.post("/predict")
def predict():
    data = request.get_json(silent=True) or {}

    # ---- Case 1: ĐÚNG format C++ gửi lên ----
    if "cpu" in data and "queue_len" in data:
        try:
            cpu = float(data.get("cpu", 0.0))
            qlen = int(data.get("queue_len", 0))

            qbin = data.get("queue_bin", None)
            if qbin is None:
                qbin = compute_queue_bin(qlen)

            row = {
                "cpu": cpu,
                "queue_len": qlen,
                "queue_bin": str(qbin),  # categorical => str
                "request_method": str(data.get("request_method", "GET")),
                "request_path_length": int(data.get("request_path_length", 1)),
                "estimated_workload": float(data.get("estimated_workload", 0.0)),
                "req_size": int(data.get("req_size", 0)),
            }

            X = pd.DataFrame([row])

            y = model.predict(X)[0]

            # Nếu model trả int label => decode bằng encoder
            if label_encoder is not None:
                try:
                    algo = label_encoder.inverse_transform([int(y)])[0]
                except Exception:
                    algo = str(y)
            else:
                algo = str(y)

            return jsonify({"algorithm": algo})

        except Exception as e:
            return jsonify({"error": f"Prediction failed: {e}"}), 500

    # ---- Case 2: Hỗ trợ format cũ: {"metrics":[...]} ----
    if "metrics" in data:
        try:
            # metrics phải đúng thứ tự theo lúc train cũ của bạn
            import numpy as np
            feats = np.array(data["metrics"]).reshape(1, -1)
            y = model.predict(feats)[0]
            return jsonify({"algorithm": str(y)})
        except Exception as e:
            return jsonify({"error": f"Legacy metrics prediction failed: {e}"}), 500
        
    print("[AI-SERVER] received:", data)
    print("[AI-SERVER] predicted:", algo)

    return jsonify({"error": "Invalid payload. Expected C++ fields or 'metrics'."}), 400


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
