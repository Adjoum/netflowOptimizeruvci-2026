#!/usr/bin/env python3
"""
server.py — Serveur Flask : pont entre l'UI web et netflow.exe
NetFlow Optimizer & Security Analyzer — ALC2101 UVCI 2025-2026

Lancement : python server.py
URL       : http://localhost:5000
"""
import subprocess, json, sys, os
from pathlib import Path
from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS

BASE = Path(__file__).parent
UI   = BASE / "ui"
DATA = BASE / "data"
BIN  = BASE / ("netflow.exe" if sys.platform == "win32" else "netflow")
PORT = 5000

app = Flask(__name__, static_folder=str(UI))
CORS(app)

def call_c(payload):
    """Envoie JSON sur stdin du binaire C, retourne JSON depuis stdout."""
    if not BIN.exists():
        return {"error": f"Binaire introuvable: {BIN}  →  lancez: make"}
    try:
        r = subprocess.run(
            [str(BIN), "--api"],
            input=json.dumps(payload),
            capture_output=True, text=True,
            timeout=15, cwd=str(BASE)
        )
        out = r.stdout.strip()
        if not out:
            return {"error": "Pas de sortie C. stderr: " + r.stderr[:200]}
        return json.loads(out)
    except subprocess.TimeoutExpired:
        return {"error": "Timeout (>15s)"}
    except json.JSONDecodeError as e:
        return {"error": f"JSON invalide: {e}", "raw": r.stdout[:300]}
    except Exception as e:
        return {"error": str(e)}

# ── UI ─────────────────────────────────────────────────────────────────────
@app.route("/")
def index():
    return send_from_directory(str(UI), "netflow-optimizer.html")


# ── Status ─────────────────────────────────────────────────────────────────
@app.route("/api/status")
def status():
    return jsonify({"ok": BIN.exists(), "binary": str(BIN),
                    "platform": sys.platform, "port": PORT})

# ── Graphe ─────────────────────────────────────────────────────────────────
@app.route("/api/graph")
def get_graph():
    return jsonify(call_c({"cmd": "get_graph"}))

@app.route("/api/graph/load", methods=["POST"])
def load_graph():
    d = request.get_json(force=True) or {}
    return jsonify(call_c({"cmd": "load_graph", "file": d.get("file", "data/reseau_demo.txt")}))

@app.route("/api/graph/edge", methods=["POST"])
def add_edge():
    d = request.get_json(force=True) or {}
    return jsonify(call_c({"cmd":"add_edge","src":int(d.get("src",0)),"dst":int(d.get("dst",1)),
                            "lat":float(d.get("lat",10)),"bw":float(d.get("bw",100)),
                            "cost":float(d.get("cost",1)),"sec":int(d.get("sec",8))}))

@app.route("/api/graph/edge", methods=["DELETE"])
def del_edge():
    d = request.get_json(force=True) or {}
    return jsonify(call_c({"cmd":"del_edge","src":int(d.get("src",0)),"dst":int(d.get("dst",1))}))

@app.route("/api/files")
def list_files():
    files = [f.name for f in DATA.glob("*.txt")] if DATA.exists() else []
    return jsonify({"files": files})

# ── Routage ────────────────────────────────────────────────────────────────
@app.route("/api/routing/dijkstra",   methods=["POST"])
def dijkstra():
    d = request.get_json(force=True) or {}
    return jsonify(call_c({"cmd":"dijkstra","src":int(d.get("src",0)),"dst":int(d.get("dst",7)),"metric":d.get("metric","latence")}))

@app.route("/api/routing/bellman",    methods=["POST"])
def bellman():
    d = request.get_json(force=True) or {}
    return jsonify(call_c({"cmd":"bellman","src":int(d.get("src",0)),"dst":int(d.get("dst",7)),"metric":d.get("metric","latence")}))

@app.route("/api/routing/compare",    methods=["POST"])
def compare():
    d = request.get_json(force=True) or {}
    return jsonify(call_c({"cmd":"compare","src":int(d.get("src",0)),"dst":int(d.get("dst",7)),"metric":d.get("metric","latence")}))

@app.route("/api/routing/kpaths",     methods=["POST"])
def kpaths():
    d = request.get_json(force=True) or {}
    return jsonify(call_c({"cmd":"kpaths","src":int(d.get("src",0)),"dst":int(d.get("dst",7)),"k":int(d.get("k",3)),"metric":d.get("metric","latence")}))

@app.route("/api/routing/constrained",methods=["POST"])
def constrained():
    d = request.get_json(force=True) or {}
    return jsonify(call_c({"cmd":"constrained","src":int(d.get("src",0)),"dst":int(d.get("dst",7)),
                            "bw_min":float(d.get("bw_min",0)),"cout_max":float(d.get("cout_max",999999)),"sec_min":int(d.get("sec_min",0))}))

# ── Sécurité ───────────────────────────────────────────────────────────────
@app.route("/api/security/dfs",         methods=["POST"])
def dfs():
    d = request.get_json(force=True) or {}
    return jsonify(call_c({"cmd":"dfs","start":int(d.get("start",0))}))

@app.route("/api/security/bfs",         methods=["POST"])
def bfs():
    d = request.get_json(force=True) or {}
    return jsonify(call_c({"cmd":"bfs","start":int(d.get("start",0))}))

@app.route("/api/security/cycles")
def cycles():      return jsonify(call_c({"cmd":"cycles"}))

@app.route("/api/security/articulation")
def articulation():return jsonify(call_c({"cmd":"articulation"}))

@app.route("/api/security/bridges")
def bridges():     return jsonify(call_c({"cmd":"bridges"}))

@app.route("/api/security/components")
def components():  return jsonify(call_c({"cmd":"components"}))

@app.route("/api/security/tarjan")
def tarjan():      return jsonify(call_c({"cmd":"tarjan"}))

@app.route("/api/security/full")
def security_full():return jsonify(call_c({"cmd":"security"}))

# ── File de paquets ────────────────────────────────────────────────────────
@app.route("/api/queue", methods=["POST"])
def queue_ops():
    d = request.get_json(force=True) or {}
    payload = {"cmd": "queue_ops"}
    payload.update(d)
    return jsonify(call_c(payload))

# ── Benchmark ──────────────────────────────────────────────────────────────
@app.route("/api/benchmark")
def benchmark():   return jsonify(call_c({"cmd":"benchmark"}))

# ── Démarrage ──────────────────────────────────────────────────────────────
if __name__ == "__main__":
    ok = "OK" if BIN.exists() else "MANQUANT — faites: make"
    print(f"""
╔══════════════════════════════════════════════════════╗
║   NetFlow Optimizer — Serveur Web                    ║
║   ALC2101 — UVCI 2025-2026                           ║
╠══════════════════════════════════════════════════════╣
║  Binaire C  : {str(BIN)[:40]:<40} ║
║  Statut     : {ok:<40} ║
║  URL        : http://localhost:{PORT:<24} ║
╚══════════════════════════════════════════════════════╝
""")
    app.run(host="0.0.0.0", port=PORT, debug=False)
