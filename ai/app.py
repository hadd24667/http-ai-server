from flask import Flask, request, jsonify
import joblib, numpy as np

app = Flask(__name__)
model = joblib.load("models/scheduler_model.pkl")

@app.route("/predict", methods=["POST"])
def predict():
    data = request.get_json()
    features = np.array(data["metrics"]).reshape(1, -1)
    pred = model.predict(features)[0]
    return jsonify({"best_algo": pred})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
