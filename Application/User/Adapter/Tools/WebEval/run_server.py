"""
Standalone server launcher that imports app.py and registers extra API routes.
Use this instead of running app.py directly to get keyword recording + training.
"""
import json
import os
import re
import struct
import subprocess
import sys
import threading
import wave
from pathlib import Path

import numpy as np

import app as _app_module

app = _app_module.app

# ── Serial port support ───────────────────────────────────────────────────────
try:
    import serial
    import serial.tools.list_ports
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WebEval] pyserial not installed. Serial features disabled. Install with: pip install pyserial")

_serial_lock = threading.Lock()
_serial_port = None  # serial.Serial instance
_serial_config = {
    "port": "",
    "baudrate": 115200,
    "connected": False,
    "threshold": 0.85,  # default 85% confidence
    "commands": {},     # keyword -> command string mapping
}

# Keep runtime data/model paths aligned with app.py and the embedded project.
DATASET_DIR = _app_module.DATASET_DIR
MODEL_DIR = _app_module.MODEL_DIR
MODEL_PATH = _app_module.MODEL_PATH
CALIB_PATH = MODEL_DIR / "my_kws_calib.npz"

# WebEval -> Tools -> Adapter -> User -> Application -> xos_ai -> stm32f412
_STM32_ROOT = Path(_app_module.BASE_DIR).parent.parent.parent.parent.parent.parent
_COMPILER_ROOT = _STM32_ROOT / "rl_sdk" / "minerva" / "compiler"

# Compiler/training scripts
TRAIN_SCRIPT = _COMPILER_ROOT / "train_kws_npz.py"
COMPILE_SCRIPT = _COMPILER_ROOT / "minerva_compile.py"
KEY_PATH = _COMPILER_ROOT / "key.bin"

# Ensure directories exist
DATASET_DIR.mkdir(parents=True, exist_ok=True)
MODEL_DIR.mkdir(parents=True, exist_ok=True)

_training_lock = threading.Lock()
_training_status = {"running": False, "message": "", "error": None}

# Path to model_data.js (same dir as run_server.py / app.py)
_MODEL_DATA_JS = Path(_app_module.BASE_DIR) / "model_data.js"


def _generate_model_data_js(model_path: Path):
    """Convert .npz model to model_data.js for frontend JS inference."""
    data = np.load(model_path, allow_pickle=True)
    result = {
        "labels": [str(x) for x in data["labels"].tolist()],
        "w0": data["layer_0_w"].astype(np.float32).tolist(),
        "b0": data["layer_0_b"].astype(np.float32).tolist(),
        "w1": data["layer_1_w"].astype(np.float32).tolist(),
        "b1": data["layer_1_b"].astype(np.float32).tolist(),
        "w2": data["layer_2_w"].astype(np.float32).tolist(),
        "b2": data["layer_2_b"].astype(np.float32).tolist(),
    }
    js_content = "window.MODEL_DATA = " + json.dumps(result) + ";\n"
    _MODEL_DATA_JS.write_text(js_content, encoding="utf-8")
    print(f"[WebEval] Updated {_MODEL_DATA_JS} with labels: {result['labels']}")
    return True


def _bootstrap_unknown_samples(rng_seed: int = 42):
    """
    Generate diverse "unknown" training samples so the model learns
    what is NOT a keyword. Without this, the model only sees corrupted
    keyword audio as unknown, causing near-100% false positives.

    Strategies:
      - White / pink-ish / band-limited noise bursts
      - Random sine-chirp "speech-like" nonsense
      - Scrambled/shuffled keyword audio (destroys temporal structure)
      - Mixed overlapping keyword fragments
    """
    UNKNOWN_DIR = DATASET_DIR / "unknown"
    # Clear old auto-generated samples
    for old in UNKNOWN_DIR.glob("auto_*.wav"):
        old.unlink()
    UNKNOWN_DIR.mkdir(parents=True, exist_ok=True)

    rng = np.random.default_rng(rng_seed)
    sr = 48000
    gen_count = 0

    # Collect keyword PCM for scrambling
    kw_pcms = []
    for kw_dir in DATASET_DIR.iterdir():
        if not kw_dir.is_dir() or kw_dir.name.lower() == "unknown":
            continue
        for wav_path in kw_dir.glob("*.wav"):
            try:
                with wave.open(str(wav_path), "rb") as wf:
                    raw = wf.readframes(wf.getnframes())
                pcm = np.frombuffer(raw, dtype="<i2").astype(np.float32)
                if len(pcm) > 480:
                    kw_pcms.append(pcm)
            except Exception:
                pass

    def _write_wav(pcm_i16: np.ndarray, name: str):
        nonlocal gen_count
        out = UNKNOWN_DIR / name
        with wave.open(str(out), "wb") as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(sr)
            wf.writeframes(pcm_i16.astype(np.int16).tobytes())
        gen_count += 1

    # 1. Pure noise variants (different colors) - expanded
    for i in range(10):
        dur = rng.integers(16000, 48000)
        noise_type = rng.integers(0, 4)
        if noise_type == 0:  # white
            pcm = rng.normal(0, 800, dur).astype(np.float32)
        elif noise_type == 1:  # pink-ish: low-pass via moving average
            raw = rng.normal(0, 1200, dur + 4).astype(np.float32)
            pcm = np.convolve(raw, np.ones(5) / 5, mode="valid")[:dur]
        elif noise_type == 2:  # band-limited: 300-3400 Hz
            raw = rng.normal(0, 1500, dur).astype(np.float32)
            from numpy.fft import rfft, irfft
            spec = rfft(raw)
            freqs = np.fft.rfftfreq(dur, 1 / sr)
            spec[(freqs < 300) | (freqs > 3400)] = 0
            pcm = irfft(spec, n=dur).astype(np.float32)
        else:  # high-pass noise (>2kHz, like mic hiss)
            raw = rng.normal(0, 600, dur).astype(np.float32)
            from numpy.fft import rfft, irfft
            spec = rfft(raw)
            freqs = np.fft.rfftfreq(dur, 1 / sr)
            spec[freqs < 2000] = 0
            pcm = irfft(spec, n=dur).astype(np.float32)
        pcm = np.clip(pcm * rng.uniform(0.5, 1.0), -32768, 32767)
        _write_wav(pcm, f"auto_noise_{i:03d}.wav")

    # 2. Sine-chirp speech-like nonsense - expanded
    for i in range(8):
        dur = rng.integers(12000, 40000)
        t = np.arange(dur, dtype=np.float32) / sr
        n_tones = rng.integers(2, 8)
        pcm = np.zeros(dur, dtype=np.float32)
        for _ in range(n_tones):
            f0 = rng.uniform(150, 3000)
            f1 = f0 * rng.uniform(0.6, 1.5)
            env_len = rng.integers(dur // 8, dur // 2)
            pos = rng.integers(0, max(1, dur - env_len))
            env = np.hanning(env_len * 2)[:env_len].astype(np.float32)
            if pos + env_len > dur:
                env = env[:dur - pos]
            freq = np.linspace(f0, f1, len(env))
            phase = np.cumsum(freq) / sr * 2 * np.pi
            pcm[pos:pos + len(env)] += np.sin(phase) * env * 0.3
        pcm = np.clip(pcm * rng.uniform(500, 1500), -32768, 32767)
        _write_wav(pcm, f"auto_synth_{i:03d}.wav")

    # 3. Scrambled keyword audio (destroys word identity, keeps speech texture) - expanded
    for i, pcm_kw in enumerate(kw_pcms[:8]):
        dur = min(len(pcm_kw), sr)
        seg = pcm_kw[:dur].copy()
        # Shuffle 5-30ms chunks at varying sizes
        chunk_sz = rng.integers(int(sr * 0.005), int(sr * 0.03))
        n_chunks = len(seg) // chunk_sz
        if n_chunks < 4:
            continue
        chunks = [seg[j * chunk_sz:(j + 1) * chunk_sz] for j in range(n_chunks)]
        rng.shuffle(chunks)
        scrambled = np.concatenate(chunks)
        scrambled = scrambled * rng.uniform(0.6, 1.4)
        scrambled = np.clip(scrambled, -32768, 32767)
        _write_wav(scrambled, f"auto_scramble_{i:03d}.wav")
        # Also generate time-reversed version
        reversed_pcm = scrambled[::-1].copy()
        reversed_pcm = np.clip(reversed_pcm * rng.uniform(0.8, 1.2), -32768, 32767)
        _write_wav(reversed_pcm, f"auto_reverse_{i:03d}.wav")

    # 4. Overlapping keyword fragments (mixed speech nonsense) - expanded
    if len(kw_pcms) >= 2:
        for i in range(min(6, len(kw_pcms) * 2)):
            p1 = kw_pcms[i % len(kw_pcms)][:sr]
            p2 = kw_pcms[(i + 1) % len(kw_pcms)][:sr]
            min_len = min(len(p1), len(p2))
            start_offset = rng.integers(0, max(1, min_len // 3))
            mixed = np.zeros(min_len + start_offset, dtype=np.float32)
            mixed[:min_len] = p1[:min_len] * rng.uniform(0.3, 0.8)
            mixed[start_offset:start_offset + min_len] += p2[:min_len] * rng.uniform(0.3, 0.8)
            mixed = np.clip(mixed, -32768, 32767)
            _write_wav(mixed, f"auto_mix_{i:03d}.wav")

    # 5. Silence with occasional clicks (ambient-like) - expanded
    for i in range(6):
        dur = rng.integers(20000, sr)
        pcm = np.zeros(dur, dtype=np.float32)
        n_clicks = rng.integers(2, 10)
        for _ in range(n_clicks):
            pos = rng.integers(0, max(1, dur - 200))
            pcm[pos:pos + rng.integers(5, 120)] += rng.uniform(-3000, 3000)
        pcm += rng.normal(0, 30, dur).astype(np.float32)
        pcm = np.clip(pcm, -32768, 32767)
        _write_wav(pcm, f"auto_ambient_{i:03d}.wav")

    # 6. Pure silence (varying lengths) -- critical: model must learn silence = unknown
    for i in range(10):
        dur = rng.integers(16000, sr)
        pcm = np.zeros(dur, dtype=np.float32)
        _write_wav(pcm, f"auto_silence_{i:03d}.wav")

    # 7. Near-silence (tiny ambient hiss, < 5 LSB) - expanded
    for i in range(8):
        dur = rng.integers(16000, sr)
        pcm = (rng.normal(0, rng.uniform(1, 5), dur)).astype(np.float32)
        pcm = np.clip(pcm, -32768, 32767)
        _write_wav(pcm, f"auto_quiet_{i:03d}.wav")

    # 8. Moderate low-level noise (peak ~30-400, matching real-world idle mic) - expanded
    for i in range(10):
        dur = rng.integers(16000, sr)
        pcm = (rng.normal(0, rng.uniform(30, 200), dur)).astype(np.float32)
        pcm = np.clip(pcm, -32768, 32767)
        _write_wav(pcm, f"auto_low_noise_{i:03d}.wav")

    # 9. Burst noise (short loud events, like door slam, keyboard click)
    for i in range(6):
        dur = rng.integers(20000, sr)
        pcm = rng.normal(0, 10, dur).astype(np.float32)  # quiet baseline
        n_bursts = rng.integers(1, 5)
        for _ in range(n_bursts):
            pos = rng.integers(0, max(1, dur - 2000))
            burst_len = rng.integers(200, 2000)
            burst = rng.normal(0, rng.uniform(2000, 8000), burst_len).astype(np.float32)
            env = np.hanning(burst_len).astype(np.float32)
            burst = burst * env
            if pos + burst_len <= dur:
                pcm[pos:pos + burst_len] += burst
        pcm = np.clip(pcm, -32768, 32767)
        _write_wav(pcm, f"auto_burst_{i:03d}.wav")

    # 10. Hum/tone noise (50/60Hz mains hum, fan drone, etc.)
    for i in range(6):
        dur = rng.integers(16000, sr)
        t = np.arange(dur, dtype=np.float32) / sr
        pcm = rng.normal(0, 20, dur).astype(np.float32)
        # Add hum fundamentals + harmonics
        base_freq = rng.choice([50, 60, 100, 120, 150, 200])
        for harm in range(1, 5):
            pcm += np.sin(2 * np.pi * base_freq * harm * t) * rng.uniform(200, 800) / harm
        pcm = np.clip(pcm, -32768, 32767)
        _write_wav(pcm, f"auto_hum_{i:03d}.wav")

    print(f"[WebEval] Bootstrapped {gen_count} diverse unknown samples in {UNKNOWN_DIR}")
    return gen_count


def sanitize_keyword(name: str) -> str:
    s = name.strip()
    if not s:
        return ""
    s = re.sub(r'[\\/:*?"<>|]', '_', s)
    s = re.sub(r'\s+', '_', s)
    s = re.sub(r'[^a-zA-Z0-9_\u4e00-\u9fff]', '', s)
    return s[:32]


# ── API routes ────────────────────────────────────────────────────────────────

@app.get("/api/keywords")
def list_keywords():
    from flask import jsonify
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
    from flask import jsonify
    return jsonify(_training_status)


@app.post("/api/record")
def record_sample():
    from flask import jsonify, request
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


@app.post("/api/delete_keyword")
def delete_keyword():
    from flask import jsonify, request
    keyword = request.form.get("keyword", "").strip()
    safe = sanitize_keyword(keyword)
    if not safe:
        return jsonify({"error": "invalid keyword name"}), 400
    kw_dir = DATASET_DIR / safe
    if not kw_dir.exists():
        return jsonify({"error": "keyword not found"}), 404
    import shutil
    shutil.rmtree(str(kw_dir))
    print(f"[WebEval] Deleted keyword '{safe}' and all its samples")
    return jsonify({"ok": True, "keyword": safe})


@app.post("/api/delete_sample")
def delete_sample():
    from flask import jsonify, request
    keyword = request.form.get("keyword", "").strip()
    filename = request.form.get("file", "").strip()
    safe = sanitize_keyword(keyword)
    if not safe or not filename:
        return jsonify({"error": "invalid params"}), 400
    filepath = DATASET_DIR / safe / filename
    if not filepath.exists():
        return jsonify({"error": "file not found"}), 404
    filepath.unlink()
    print(f"[WebEval] Deleted sample {filename} from '{safe}'")
    return jsonify({"ok": True, "keyword": safe, "file": filename})


@app.post("/api/compile")
def trigger_compile():
    """Compile existing .npz model to STM32 embedded weights (C headers)."""
    from flask import jsonify

    if not MODEL_PATH.exists():
        return jsonify({"error": "No trained model found. Train first."}), 404

    if not _training_lock.acquire(blocking=False):
        return jsonify({"error": "training/compilation already in progress"}), 409

    _training_status["running"] = True
    _training_status["message"] = "Compiling STM32 weights..."
    _training_status["error"] = None

    def _run_compile():
        try:
            if not KEY_PATH.exists():
                _training_status["message"] = "Generating encryption key..."
                KEY_PATH.parent.mkdir(parents=True, exist_ok=True)
                KEY_PATH.write_bytes(os.urandom(32))
                print(f"[WebEval] Generated key at {KEY_PATH}")

            print("[WebEval] Running minerva_compile.py ...")
            cmd = [sys.executable, str(COMPILE_SCRIPT),
                   str(MODEL_PATH),
                   "--key", str(KEY_PATH),
                   "--target", "stm32f4",
                   "--out-dir", str(MODEL_DIR)]
            if CALIB_PATH.exists():
                cmd.insert(5, "--calibrate")
                cmd.insert(6, str(CALIB_PATH))
            else:
                print("[WebEval] No calibration file, compiling without PTQ calibration")
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
            print("[WebEval] compile stdout:", result.stdout)
            if result.returncode != 0:
                print("[WebEval] compile stderr:", result.stderr)
                _training_status["error"] = f"Compilation failed: {result.stderr[:500]}"
                return

            # List generated files
            generated = []
            for fname in ["weights.c", "weights.h", "labels.h", "mnv_model_config.h"]:
                fp = MODEL_DIR / fname
                if fp.exists():
                    generated.append({"name": fname, "size": fp.stat().st_size})

            _training_status["message"] = (
                f"Done. Generated {len(generated)} files in {MODEL_DIR}"
            )
            print(f"[WebEval] Compilation complete. Files: {[g['name'] for g in generated]}")

        except subprocess.TimeoutExpired:
            _training_status["error"] = "Compilation timed out (2 min limit)"
        except Exception as e:
            _training_status["error"] = f"Compilation error: {str(e)}"
            print(f"[WebEval] Compilation exception: {e}")
        finally:
            _training_status["running"] = False
            _training_lock.release()

    threading.Thread(target=_run_compile, daemon=True).start()
    return jsonify({"ok": True, "message": "Compilation started in background"})


@app.post("/api/train")
def trigger_train():
    from flask import jsonify, request

    if not _training_lock.acquire(blocking=False):
        return jsonify({"error": "training already in progress"}), 409

    _training_status["running"] = True
    _training_status["message"] = "Starting training..."
    _training_status["error"] = None

    def _run_training():
        try:
            keyword_dirs = [d for d in DATASET_DIR.iterdir() if d.is_dir()]
            total_samples = sum(len(list(d.glob("*.wav"))) for d in keyword_dirs)
            if total_samples < 10:
                _training_status["error"] = f"样本不足 ({total_samples})。建议每个关键词至少录制10条样本，且务必录制非关键词样本，否则识别率很低。"
                return

            if not KEY_PATH.exists():
                _training_status["message"] = "Generating encryption key..."
                KEY_PATH.parent.mkdir(parents=True, exist_ok=True)
                KEY_PATH.write_bytes(os.urandom(32))
                print(f"[WebEval] Generated key at {KEY_PATH}")

            MODEL_DIR.mkdir(parents=True, exist_ok=True)

            _training_status["message"] = "Generating diverse unknown samples..."
            try:
                n_unknown = _bootstrap_unknown_samples()
                print(f"[WebEval] Generated {n_unknown} bootstrapped unknown samples")
            except Exception as e:
                print(f"[WebEval] WARNING: unknown bootstrap failed: {e}")

            _training_status["message"] = "Training model..."
            print("[WebEval] Running train_kws_npz.py ...")
            result = subprocess.run(
                [sys.executable, str(TRAIN_SCRIPT),
                 "--data-dir", str(DATASET_DIR),
                 "--out-model", str(MODEL_PATH),
                 "--out-calib", str(CALIB_PATH),
                 "--epochs", "800",
                 "--lr", "0.001",
                 "--hidden1", "48",
                 "--hidden2", "24"],
                capture_output=True, text=True, timeout=600,
            )
            print("[WebEval] train stdout:", result.stdout)
            if result.returncode != 0:
                print("[WebEval] train stderr:", result.stderr)
                _training_status["error"] = f"Training failed: {result.stderr[:500]}"
                return

            _training_status["message"] = "Compiling model..."
            print("[WebEval] Running minerva_compile.py ...")
            cmd2 = [sys.executable, str(COMPILE_SCRIPT),
                    str(MODEL_PATH),
                    "--key", str(KEY_PATH),
                    "--target", "stm32f4",
                    "--out-dir", str(MODEL_DIR)]
            if CALIB_PATH.exists():
                cmd2.insert(5, "--calibrate")
                cmd2.insert(6, str(CALIB_PATH))
            result2 = subprocess.run(cmd2, capture_output=True, text=True, timeout=120)
            print("[WebEval] compile stdout:", result2.stdout)
            if result2.returncode != 0:
                print("[WebEval] compile stderr:", result2.stderr)
                _training_status["error"] = f"Compilation failed: {result2.stderr[:500]}"
                return

            _training_status["message"] = "Updating frontend model_data.js..."
            try:
                _generate_model_data_js(MODEL_PATH)
            except Exception as e:
                print(f"[WebEval] WARNING: Failed to update model_data.js: {e}")

            _training_status["message"] = "Reloading model..."
            try:
                _app_module.runner = _app_module.ModelRunner(MODEL_PATH)
                print(f"[WebEval] Model reloaded, labels: {_app_module.runner.labels}")
            except Exception as e:
                _training_status["error"] = f"Model reload failed: {e}"
                return

            _training_status["message"] = f"Done. Model trained with labels: {_app_module.runner.labels}"
            print(f"[WebEval] Training complete. Labels: {_app_module.runner.labels}")

        except subprocess.TimeoutExpired:
            _training_status["error"] = "Training timed out (10 min limit)"
        except Exception as e:
            _training_status["error"] = f"Training error: {str(e)}"
            print(f"[WebEval] Training exception: {e}")
        finally:
            _training_status["running"] = False
            _training_lock.release()

    threading.Thread(target=_run_training, daemon=True).start()
    return jsonify({"ok": True, "message": "Training started in background"})


# ── Serial API routes ──────────────────────────────────────────────────────────

@app.get("/api/serial/ports")
def serial_list_ports():
    """List available serial ports."""
    from flask import jsonify
    if not HAS_SERIAL:
        return jsonify({"error": "pyserial not installed", "ports": []})
    ports = []
    try:
        for p in serial.tools.list_ports.comports():
            ports.append({
                "device": p.device,
                "description": p.description,
                "hwid": p.hwid,
            })
    except Exception as e:
        return jsonify({"error": str(e), "ports": ports})
    return jsonify({"ports": ports})


@app.get("/api/serial/config")
def serial_get_config():
    """Get current serial configuration."""
    from flask import jsonify
    return jsonify(_serial_config)


@app.post("/api/serial/connect")
def serial_connect():
    """Connect to a serial port."""
    from flask import jsonify, request
    if not HAS_SERIAL:
        return jsonify({"error": "pyserial not installed"}), 500

    data = request.get_json(silent=True) or {}
    port = data.get("port", "").strip()
    baudrate = int(data.get("baudrate", 115200))
    threshold = float(data.get("threshold", 0.85))
    commands = data.get("commands", {})

    if not port:
        return jsonify({"error": "no port specified"}), 400

    with _serial_lock:
        # Disconnect first if already connected
        if _serial_port is not None and _serial_port.is_open:
            try:
                _serial_port.close()
            except Exception:
                pass
            _serial_port = None

        try:
            sp = serial.Serial(port=port, baudrate=baudrate, timeout=1, write_timeout=1)
            _serial_port = sp
            _serial_config["port"] = port
            _serial_config["baudrate"] = baudrate
            _serial_config["threshold"] = threshold
            _serial_config["commands"] = commands
            _serial_config["connected"] = True
            print(f"[WebEval] Serial connected: {port} @ {baudrate} baud, threshold={threshold}")
            return jsonify({"ok": True, "config": _serial_config})
        except Exception as e:
            _serial_config["connected"] = False
            _serial_port = None
            return jsonify({"error": f"Connect failed: {str(e)}"}), 500


@app.post("/api/serial/disconnect")
def serial_disconnect():
    """Disconnect from serial port."""
    from flask import jsonify
    with _serial_lock:
        if _serial_port is not None and _serial_port.is_open:
            try:
                _serial_port.close()
            except Exception:
                pass
        _serial_port = None
        _serial_config["connected"] = False
        _serial_config["port"] = ""
    print("[WebEval] Serial disconnected")
    return jsonify({"ok": True})


@app.post("/api/serial/send")
def serial_send():
    """Send a command via serial port."""
    from flask import jsonify, request
    if not HAS_SERIAL:
        return jsonify({"error": "pyserial not installed"}), 500

    data = request.get_json(silent=True) or {}
    command = data.get("command", "")

    if not command:
        return jsonify({"error": "no command specified"}), 400

    with _serial_lock:
        if _serial_port is None or not _serial_port.is_open:
            return jsonify({"error": "serial not connected"}), 503
        try:
            # Support \r, \n, \r\n in command string
            cmd_bytes = command.encode("utf-8")
            _serial_port.write(cmd_bytes)
            _serial_port.flush()
            print(f"[WebEval] Serial sent: {repr(command)}")
            return jsonify({"ok": True, "sent": command})
        except Exception as e:
            return jsonify({"error": f"Send failed: {str(e)}"}), 500


@app.post("/api/serial/update_config")
def serial_update_config():
    """Update threshold and commands without reconnecting."""
    from flask import jsonify, request
    data = request.get_json(silent=True) or {}
    if "threshold" in data:
        _serial_config["threshold"] = float(data["threshold"])
    if "commands" in data:
        _serial_config["commands"] = data["commands"]
    print(f"[WebEval] Serial config updated: threshold={_serial_config['threshold']}")
    return jsonify({"ok": True, "config": _serial_config})


@app.post("/api/serial/notify_keyword")
def serial_notify_keyword():
    """
    Called by the frontend when a keyword is recognized with sufficient confidence.
    If confidence >= threshold and serial is connected, sends the configured command.
    """
    from flask import jsonify, request
    data = request.get_json(silent=True) or {}
    keyword = data.get("keyword", "").strip()
    confidence = float(data.get("confidence", 0))

    if not keyword:
        return jsonify({"ok": False, "reason": "no keyword"})

    threshold = _serial_config.get("threshold", 0.85)
    if confidence < threshold:
        return jsonify({"ok": False, "reason": f"confidence {confidence:.3f} < threshold {threshold}"})

    commands = _serial_config.get("commands", {})
    cmd = commands.get(keyword, "")
    if not cmd:
        return jsonify({"ok": False, "reason": f"no command configured for keyword '{keyword}'"})

    with _serial_lock:
        if _serial_port is None or not _serial_port.is_open:
            return jsonify({"ok": False, "reason": "serial not connected"})
        try:
            cmd_bytes = cmd.encode("utf-8")
            _serial_port.write(cmd_bytes)
            _serial_port.flush()
            print(f"[WebEval] Keyword '{keyword}' ({confidence:.1%}) -> serial: {repr(cmd)}")
            return jsonify({"ok": True, "sent": cmd})
        except Exception as e:
            return jsonify({"error": f"Send failed: {str(e)}"}), 500


def start_server():
    print(f"[WebEval] Paths:")
    print(f"  DATASET_DIR = {DATASET_DIR}")
    print(f"  MODEL_DIR   = {MODEL_DIR}")
    print(f"  MODEL_PATH  = {MODEL_PATH}")
    print(f"  TRAIN_SCRIPT= {TRAIN_SCRIPT}")
    print(f"  COMPILE_SCRIPT={COMPILE_SCRIPT}")

    # Re-initialize runner with correct MODEL_PATH
    if MODEL_PATH.exists():
        try:
            _app_module.runner = _app_module.ModelRunner(MODEL_PATH)
            print(f"[WebEval] Model loaded from {MODEL_PATH}, labels: {_app_module.runner.labels}")
            # Sync frontend model_data.js with current model
            _generate_model_data_js(MODEL_PATH)
        except Exception as e:
            print(f"[WebEval] WARNING: Failed to load model: {e}")
    else:
        print("[WebEval] No existing model found. Train first before running inference.")

    print("[WebEval] Starting with extended routes (recording + training)")
    app.run(host="127.0.0.1", port=8787, debug=False)


if __name__ == "__main__":
    start_server()
