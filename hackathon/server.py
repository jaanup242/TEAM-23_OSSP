#!/usr/bin/env python3
"""MemTrace bridge: serves the web UI and runs the compiled C engine for each request.
Only the Python standard library is used (preinstalled on Ubuntu)."""
import json, os, platform, subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENGINE = os.path.join(ROOT, "build", "memtrace")
TESTS = os.path.join(ROOT, "build", "test_engine")
PAGE = os.path.join(ROOT, "web", "index.html")
HOST, PORT = "127.0.0.1", int(os.environ.get("PORT", "8000"))


def os_name():
    try:
        with open("/etc/os-release") as f:
            for line in f:
                if line.startswith("PRETTY_NAME="):
                    return line.split("=", 1)[1].strip().strip('"')
    except OSError:
        pass
    return platform.system()


def to_script(d):
    """Turn the JSON request into the engine's text script. Everything is checked to be an int first."""
    def integer(v, lo, hi, name):
        if isinstance(v, bool) or not isinstance(v, int) or not lo <= v <= hi:
            raise ValueError(f"{name} must be an integer from {lo} to {hi}")
        return v
    if d.get("policy") not in ("LRU", "FIFO"):
        raise ValueError("policy must be LRU or FIFO")
    out = [f"pagesize {integer(d.get('pageSize'), 2, 65536, 'pageSize')}",
           f"pages {integer(d.get('pages'), 1, 1024, 'pages')}",
           f"tlb {integer(d.get('tlb'), 1, 64, 'tlb')}",
           f"policy {d['policy']}"]
    for pair in d.get("map", []):
        if not isinstance(pair, list) or len(pair) != 2:
            raise ValueError("map entries must be [page, frame]")
        out.append(f"map {integer(pair[0], 0, 1023, 'page')} {integer(pair[1], 0, 10**6, 'frame')}")
    accesses = d.get("accesses", [])
    if not isinstance(accesses, list) or not 1 <= len(accesses) <= 5000:
        raise ValueError("give between 1 and 5000 addresses")
    out += [f"access {integer(a, -2**40, 2**40, 'address')}" for a in accesses]
    return "\n".join(out) + "\n"


def run(cmd, stdin=""):
    if not os.path.exists(cmd):
        raise FileNotFoundError("C engine not built. Run `make` in the project folder first.")
    return subprocess.run([cmd], input=stdin, capture_output=True, text=True, timeout=5)


class Handler(BaseHTTPRequestHandler):
    def reply(self, code, body, ctype="application/json"):
        data = body if isinstance(body, bytes) else body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def err(self, code, msg):
        self.reply(code, json.dumps({"error": msg}))

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            with open(PAGE, "rb") as f:
                return self.reply(200, f.read(), "text/html; charset=utf-8")
        if self.path == "/api/info":
            return self.reply(200, json.dumps({"os": os_name(), "kernel": platform.release(),
                                               "engine": os.path.exists(ENGINE)}))
        self.err(404, "not found")

    def do_POST(self):
        try:
            if self.path == "/api/simulate":
                size = int(self.headers.get("Content-Length", 0))
                script = to_script(json.loads(self.rfile.read(size) or b"{}"))
                p = run(ENGINE, script)
                return self.reply(200 if p.returncode == 0 else 400, p.stdout or json.dumps({"error": p.stderr}))
            if self.path == "/api/test":
                p = run(TESTS)
                return self.reply(200, json.dumps({"passed": p.returncode == 0, "output": p.stdout}))
            self.err(404, "not found")
        except (ValueError, json.JSONDecodeError) as e:
            self.err(400, str(e))
        except FileNotFoundError as e:
            self.err(503, str(e))
        except subprocess.TimeoutExpired:
            self.err(500, "engine timed out")

    def log_message(self, fmt, *args):
        print("%s %s" % (self.address_string(), fmt % args))


if __name__ == "__main__":
    print(f"MemTrace on {os_name()} -> http://localhost:{PORT}")
    ThreadingHTTPServer((HOST, PORT), Handler).serve_forever()
