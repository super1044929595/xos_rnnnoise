from pathlib import Path
import sys
import tempfile
import wave

import numpy as np
from flask import Flask, jsonify, request, send_from_directory


BASE_DIR = Path(__file__).resolve().parent
MODEL_DIR = BASE_DIR.parent.parent / "RlSdk" / "minerva" / "generated"
MODEL_PATH = MODEL_DIR / "my_kws_model.npz"

INPUT_FEATURES = 36
TARGET_SR = 48000
DECIM_STRIDE = 3
FRAME_SAMPLES = 480
FFT_SIZE = 512
MFCC_COUNT = 12
MEL_BINS = 20
MEL_SUMMARY = 16
MEL_EDGES = [9, 13, 16, 21, 25, 31, 37, 43, 50, 58, 67, 77, 87, 99, 113, 127, 144, 162, 182, 204, 229, 256]
DCT_Q8 = np.asarray([
    [256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256],
    [255, 249, 237, 218, 195, 166, 134, 98, 60, 20, -20, -60, -98, -134, -166, -195, -218, -237, -249, -255],
    [253, 228, 181, 116, 40, -40, -116, -181, -228, -253, -253, -228, -181, -116, -40, 40, 116, 181, 228, 253],
    [249, 195, 98, -20, -134, -218, -255, -237, -166, -60, 60, 166, 237, 255, 218, 134, 20, -98, -195, -249],
    [243, 150, 0, -150, -243, -243, -150, 0, 150, 243, 243, 150, 0, -150, -243, -243, -150, 0, 150, 243],
    [237, 98, -98, -237, -237, -98, 98, 237, 237, 98, -98, -237, -237, -98, 98, 237, 237, 98, -98, -237],
    [228, 40, -181, -253, -116, 116, 253, 181, -40, -228, -228, -40, 181, 253, 116, -116, -253, -181, 40, 228],
    [218, -20, -237, -195, 60, 249, 166, -98, -255, -134, 134, 255, 98, -166, -249, -60, 195, 237, 20, -218],
    [207, -79, -256, -79, 207, 207, -79, -256, -79, 207, 207, -79, -256, -79, 207, 207, -79, -256, -79, 207],
    [195, -134, -237, 60, 255, 20, -249, -98, 218, 166, -166, -218, 98, 249, -20, -255, -60, 237, 134, -195],
    [181, -181, -181, 181, 181, -181, -181, 181, 181, -181, -181, 181, 181, -181, -181, 181, 181, -181, -181, 181],
    [166, -218, -98, 249, 20, -255, 60, 237, -134, -195, 195, 134, -237, -60, 255, -20, -249, 98, 218, -166],
], dtype=np.int32)


def clamp_s8(v: int) -> int:
    if v > 127:
        return 127
    if v < -128:
        return -128
    return int(v)


def log2_u32(v: int) -> int:
    r = 0
    while v > 1:
        v >>= 1
        r += 1
    return r


def linear_resample(x: np.ndarray, src_sr: int, dst_sr: int) -> np.ndarray:
    if src_sr == dst_sr:
        return x
    src_idx = np.arange(len(x), dtype=np.float32)
    dst_len = max(1, int(round(len(x) * dst_sr / src_sr)))
    dst_idx = np.linspace(0, len(x) - 1, dst_len, dtype=np.float32)
    return np.interp(dst_idx, src_idx, x).astype(np.float32)


def prepare_frame(pcm: np.ndarray) -> np.ndarray:
    if len(pcm) < FRAME_SAMPLES:
        pcm = np.concatenate([pcm, np.zeros(FRAME_SAMPLES - len(pcm), dtype=np.float32)])
    elif len(pcm) > FRAME_SAMPLES:
        start = max(0, (len(pcm) - FRAME_SAMPLES) // 2)
        pcm = pcm[start:start + FRAME_SAMPLES]
    return pcm.astype(np.int16)


def runtime_like_rfft_features(frame: np.ndarray) -> np.ndarray:
    decimated = np.zeros(FFT_SIZE, dtype=np.float32)
    energy_acc = 0
    zcr = 0
    peak = 0
    prev = int(frame[0])

    for i in range(FRAME_SAMPLES // DECIM_STRIDE):
        idx = i * DECIM_STRIDE
        sample = int(frame[idx])
        decimated[i] = sample
        energy_acc += abs(sample)
        peak = max(peak, abs(sample))
        if (prev < 0 <= sample) or (prev >= 0 > sample):
            zcr += 1
        prev = sample

    spec = np.fft.rfft(decimated, n=FFT_SIZE)
    mag = np.abs(spec).astype(np.int64)
    mel = np.zeros(MEL_BINS, dtype=np.int64)

    for band in range(MEL_BINS):
        start = MEL_EDGES[band]
        center = MEL_EDGES[band + 1]
        end = MEL_EDGES[band + 2]
        for bin_idx in range(start, end):
            weight = bin_idx - start + 1 if bin_idx < center else end - bin_idx
            mel[band] += int(mag[bin_idx]) * weight

    feat = np.zeros(INPUT_FEATURES, dtype=np.float32)
    mel_logs = np.array([log2_u32((int(v) >> 12) + 1) for v in mel], dtype=np.int32)

    for i in range(MFCC_COUNT):
        acc = int(np.sum(DCT_Q8[i] * mel_logs))
        feat[i] = clamp_s8(acc >> 6)

    low_energy = 0
    mid_energy = 0
    high_energy = 0
    flux = 0
    centroid_num = 0
    centroid_den = 0
    rolloff_den = 0
    rolloff_acc = 0
    rolloff_bin = 0
    prev_bin = 0

    for bin_idx in range(1, FFT_SIZE // 2 + 1):
        m = int(mag[bin_idx]) >> 8
        centroid_num += bin_idx * m
        centroid_den += m
        rolloff_den += m
        if bin_idx < 32:
            low_energy += m
        elif bin_idx < 96:
            mid_energy += m
        else:
            high_energy += m
        if bin_idx > 1:
            flux += abs(m - prev_bin)
        prev_bin = m

    for bin_idx in range(1, FFT_SIZE // 2 + 1):
        m = int(mag[bin_idx]) >> 8
        rolloff_acc += m
        if rolloff_acc * 20 >= rolloff_den * 17:
            rolloff_bin = bin_idx
            break

    feat[12] = clamp_s8(energy_acc // (FRAME_SAMPLES // DECIM_STRIDE))
    feat[13] = min(zcr, 127)
    feat[14] = clamp_s8(log2_u32(energy_acc + 1) * 8)
    feat[15] = clamp_s8(peak >> 8)
    feat[16] = clamp_s8(log2_u32(low_energy + 1) * 8 - 64)
    feat[17] = clamp_s8(log2_u32(mid_energy + 1) * 8 - 64)
    feat[18] = clamp_s8(log2_u32(high_energy + 1) * 8 - 64)
    feat[19] = clamp_s8(0 if centroid_den == 0 else (centroid_num * 127) // (centroid_den * (FFT_SIZE // 2)))
    feat[20] = clamp_s8((rolloff_bin * 127) // (FFT_SIZE // 2))
    feat[21] = clamp_s8(log2_u32(flux + 1) * 8 - 64)
    feat[22] = clamp_s8(0 if peak == 0 else ((energy_acc // (FRAME_SAMPLES // DECIM_STRIDE)) * 127) // peak)
    feat[23] = clamp_s8(log2_u32(rolloff_den + 1) * 8 - 64)

    summary_slots = INPUT_FEATURES - 24
    for band in range(min(MEL_SUMMARY, summary_slots)):
        src = (band * MEL_BINS) // MEL_SUMMARY
        src_next = ((band + 1) * MEL_BINS) // MEL_SUMMARY
        segment = mel[src:src_next]
        if len(segment) == 0:
            segment = np.array([0], dtype=np.int64)
        logv = log2_u32((int(np.mean(segment)) >> 12) + 1)
        feat[24 + band] = 63.0 if logv > 15 else float(logv * 8 - 64)

    return feat


def softmax(x: np.ndarray) -> np.ndarray:
    x = x - np.max(x, axis=1, keepdims=True)
    e = np.exp(x)
    return e / np.sum(e, axis=1, keepdims=True)


class ModelRunner:
    def __init__(self, model_path: Path):
        data = np.load(model_path, allow_pickle=True)
        self.w0 = data["layer_0_w"].astype(np.float32)
        self.b0 = data["layer_0_b"].astype(np.float32)
        self.w1 = data["layer_1_w"].astype(np.float32)
        self.b1 = data["layer_1_b"].astype(np.float32)
        self.w2 = data["layer_2_w"].astype(np.float32)
        self.b2 = data["layer_2_b"].astype(np.float32)
        self.labels = [str(x) for x in data["labels"].tolist()]

    def infer(self, feat: np.ndarray):
        x = feat.reshape(1, -1).astype(np.float32)
        a0 = np.maximum(0.0, x @ self.w0 + self.b0)
        a1 = np.maximum(0.0, a0 @ self.w1 + self.b1)
        logits = a1 @ self.w2 + self.b2
        probs = softmax(logits)[0]
        idx = int(np.argmax(probs))
        return {
            "label": self.labels[idx],
            "confidence": float(probs[idx]),
            "scores": {self.labels[i]: float(probs[i]) for i in range(len(self.labels))},
        }


app = Flask(__name__)
DATASET_DIR = BASE_DIR.parent.parent / "RlSdk" / "minerva" / "dataset"

runner = None
try:
    runner = ModelRunner(MODEL_PATH)
    print(f"[WebEval] Model loaded from {MODEL_PATH}, labels: {runner.labels}")
except Exception as e:
    print(f"[WebEval] WARNING: Failed to load model from {MODEL_PATH}: {e}")
    print("[WebEval] Server will start but /infer and /batch_eval will return errors.")


@app.after_request
def add_cors_headers(resp):
    resp.headers["Access-Control-Allow-Origin"] = "*"
    resp.headers["Access-Control-Allow-Headers"] = "*"
    resp.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
    return resp


@app.get("/")
def index():
    return send_from_directory(BASE_DIR, "index.html")
@app.get("/ping")
def ping():
    from flask import jsonify
    return jsonify({"ok": True, "model_loaded": runner is not None})


@app.get("/<path:filename>")
def static_files(filename):
    return send_from_directory(BASE_DIR, filename)


@app.post("/infer")
def infer():
    if runner is None:
        return jsonify({"error": "model not loaded"}), 503

    if "audio" not in request.files:
        return jsonify({"error": "missing audio file"}), 400

    audio = request.files["audio"]
    tmp_path = None
    try:
        with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as tmp:
            audio.save(tmp.name)
            tmp_path = Path(tmp.name)

        with wave.open(str(tmp_path), "rb") as wf:
            channels = wf.getnchannels()
            sampwidth = wf.getsampwidth()
            framerate = wf.getframerate()
            raw = wf.readframes(wf.getnframes())

        if sampwidth != 2:
            return jsonify({"error": "only 16-bit PCM wav is supported"}), 400

        pcm = np.frombuffer(raw, dtype="<i2").astype(np.float32)
        if channels > 1:
            pcm = pcm.reshape(-1, channels).mean(axis=1)
        pcm = linear_resample(pcm, framerate, TARGET_SR)
        frame = prepare_frame(pcm)
        feat = runtime_like_rfft_features(frame)
        result = runner.infer(feat)
        result["sample_rate"] = TARGET_SR
        result["samples"] = int(len(frame))
        result["peak"] = int(np.max(np.abs(frame)))
        return jsonify(result)
    finally:
        if tmp_path is not None:
            try:
                tmp_path.unlink()
            except Exception:
                pass


@app.get("/batch_eval")
def batch_eval():
    if runner is None:
        return jsonify({"error": "model not loaded"}), 503

    if not DATASET_DIR.exists():
        return jsonify({"error": "dataset directory not found"}), 404

    total = 0
    correct = 0
    details = []

    for label_dir in sorted([p for p in DATASET_DIR.iterdir() if p.is_dir()]):
        expected = label_dir.name
        for wav_path in sorted(label_dir.glob("*.wav")):
            with wave.open(str(wav_path), "rb") as wf:
                channels = wf.getnchannels()
                sampwidth = wf.getsampwidth()
                framerate = wf.getframerate()
                raw = wf.readframes(wf.getnframes())

            if sampwidth != 2:
                continue

            pcm = np.frombuffer(raw, dtype="<i2").astype(np.float32)
            if channels > 1:
                pcm = pcm.reshape(-1, channels).mean(axis=1)
            pcm = linear_resample(pcm, framerate, TARGET_SR)
            frame = prepare_frame(pcm)
            feat = runtime_like_rfft_features(frame)
            result = runner.infer(feat)

            total += 1
            if result["label"] == expected:
                correct += 1

            details.append({
                "file": wav_path.name,
                "expected": expected,
                "predicted": result["label"],
                "confidence": result["confidence"],
            })

    accuracy = 0.0 if total == 0 else correct / total
    return jsonify({
        "total": total,
        "correct": correct,
        "accuracy": accuracy,
        "details": details[:40],
    })


if __name__ == "__main__":
    # When app.py is launched directly, expose this module as "app" so
    # run_server.py can attach the extended routes onto the same Flask app.
    sys.modules["app"] = sys.modules[__name__]
    import run_server

    run_server.start_server()
