"""
WebEval extension: keyword recording, training, and model reloading routes.
This file is imported by app.py to add extra API endpoints.
"""
import os
import re
import subprocess
import sys
import threading
from pathlib import Path

from flask import jsonify, request

# Re-use app.py globals via import
from app import (
    app, DATASET_DIR, MODEL_DIR, MODEL_PATH, CALIB_PATH, runner, load_model,
)

# Locate compiler scripts
_PROJECT_ROOT = Path(app.root_path).parent.parent.parent.parent  # Tools -> Adapter -> User -> Application -> xos_ai
_RL_SDK = _PROJECT_ROOT.parent / "rl_sdk" / "minerva" / "compiler"
TRAIN_SCRIPT = _RL_SDK / "train_kws_npz.py"
COMPILE_SCRIPT = _RL_SDK / "minerva_compile.py"
KEY_PATH = _RL_SDK / "key.bin"

_training_lock = threading.Lock()
_training_status = {"running": False, "message": "", "error": None}


def sanitize_keyword(name: str) -> str:
    s = name.strip()
    if not s:
        return ""
    s = re.sub(r'[\\/:*?"<>|]', '_', s)
    s = re.sub(r'\s+', '_', s)
    s = re.sub(r'[^a-zA-Z0-9_\u4e00-\u9fff]', '', s)
    return s[:32]


@app.get("/api/keywords")
def list_keywords():
    keywords = []
    if DATASET_DIR.exists():
        for d in sorted(DATASET_DIR.iterdir()):
            if d.is_dir():
                wavs = sorted(d.glob("*.wav"))
                keywords.append({
                    "name": d.name,
                    "samples": len(wavs),
                    "files": [w.name for w in wavs],
                })
    return jsonify({"keywords": keywords})


@app.get("/api/train_status")
def train_status():
    return jsonify(_training_status)


@app.post("/api/record")
def record_sample():
    keyword = request.form.get("keyword", "").strip()
    safe = sanitize_keyword(keyword)
    if not safe:
        return jsonify({"error": "invalid keyword name"}), 400

    if "audio" not in request.files:
        return jsonify({"error": "missing audio file"}), 400

    kw_dir = DATASET_DIR / safe
    kw_dir.mkdir(parents=True, exist_ok=True)

    existing = sorted(kw_dir.glob("seg_*.wav"))
    next_idx = 0
    if existing:
        nums = []
        for p in existing:
            try:
                nums.append(int(p.stem.split("_")[-1]))
            except ValueError:
                pass
        next_idx = max(nums) + 1 if nums else len(existing)

    out_path = kw_dir / f"seg_{next_idx:03d}.wav"
    audio = request.files["audio"]
    audio.save(str(out_path))

    total = len(sorted(kw_dir.glob("*.wav")))
    print(f"[WebEval] Recorded {out_path.name} for keyword '{safe}' (total: {total})")
    return jsonify({
        "ok": True,
        "keyword": safe,
        "file": out_path.name,
        "total_samples": total,
    })


@app.post("/api/train")
def trigger_train():
    global runner

    if not _training_lock.acquire(blocking=False):
        return jsonify({"error": "training already in progress"}), 409

    _training_status["running"] = True
    _training_status["message"] = "Starting training..."
    _training_status["error"] = None

    def _run_training():
        from app import runner as _runner, load_model as _load_model
        nonlocal runner
        try:
            keyword_dirs = [d for d in DATASET_DIR.iterdir() if d.is_dir()]
            total_samples = sum(len(list(d.glob("*.wav"))) for d in keyword_dirs)
            if total_samples < 10:
                _training_status["error"] = f"Too few samples ({total_samples}). Need at least 10."
                return

            if not KEY_PATH.exists():
                _training_status["message"] = "Generating encryption key..."
                KEY_PATH.parent.mkdir(parents=True, exist_ok=True)
                KEY_PATH.write_bytes(os.urandom(32))
                print(f"[WebEval] Generated key at {KEY_PATH}")

            MODEL_DIR.mkdir(parents=True, exist_ok=True)

            _training_status["message"] = "Training model..."
            print("[WebEval] Running train_kws_npz.py ...")
            result = subprocess.run(
                [sys.executable, str(TRAIN_SCRIPT),
                 "--data-dir", str(DATASET_DIR),
                 "--out-model", str(MODEL_PATH),
                 "--out-calib", str(CALIB_PATH),
                 "--epochs", "200"],
                capture_output=True, text=True, timeout=300,
            )
            print("[WebEval] train stdout:", result.stdout)
            if result.returncode != 0:
                print("[WebEval] train stderr:", result.stderr)
                _training_status["error"] = f"Training failed: {result.stderr[:500]}"
                return

            _training_status["message"] = "Compiling model..."
            print("[WebEval] Running minerva_compile.py ...")
            result2 = subprocess.run(
                [sys.executable, str(COMPILE_SCRIPT),
                 str(MODEL_PATH),
                 "--key", str(KEY_PATH),
                 "--target", "stm32f4",
                 "--calibrate", str(CALIB_PATH),
                 "--out-dir", str(MODEL_DIR)],
                capture_output=True, text=True, timeout=120,
            )
            print("[WebEval] compile stdout:", result2.stdout)
            if result2.returncode != 0:
                print("[WebEval] compile stderr:", result2.stderr)
                _training_status["error"] = f"Compilation failed: {result2.stderr[:500]}"
                return

            _training_status["message"] = "Reloading model..."
            _load_model()
            # Update global reference
            import app as _app
            runner = _app.runner
            if runner is None:
                _training_status["error"] = "Model reload failed after training"
                return

            _training_status["message"] = f"Done. Model trained with labels: {runner.labels}"
            print(f"[WebEval] Training complete. Labels: {runner.labels}")

        except subprocess.TimeoutExpired:
            _training_status["error"] = "Training timed out (5 min limit)"
        except Exception as e:
            _training_status["error"] = f"Training error: {str(e)}"
            print(f"[WebEval] Training exception: {e}")
        finally:
            _training_status["running"] = False
            _training_lock.release()

    threading.Thread(target=_run_training, daemon=True).start()
    return jsonify({"ok": True, "message": "Training started in background"})
