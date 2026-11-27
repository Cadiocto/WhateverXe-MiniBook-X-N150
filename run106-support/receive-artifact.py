#!/usr/bin/env python3
"""Small, bounded receiver for the one RUN106 OpenIndiana guest.

The QEMU user-network guest reaches the GitHub runner at 192.168.122.1.
Only two fixed POST paths are accepted; uploads are written atomically.
"""
from __future__ import annotations
import argparse
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
import os
import tempfile

class Handler(BaseHTTPRequestHandler):
    root: Path
    max_bytes = 64 * 1024 * 1024

    def do_POST(self):
        names = {
            "/preflight": "target-kernel-preflight.txt",
            "/guest-artifacts": "guest-artifacts.tar.gz",
        }
        name = names.get(self.path)
        if name is None:
            self.send_error(404)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self.send_error(400, "invalid Content-Length")
            return
        if length <= 0 or length > self.max_bytes:
            self.send_error(413, "upload size outside allowed range")
            return
        data = self.rfile.read(length)
        if len(data) != length:
            self.send_error(400, "truncated upload")
            return
        self.root.mkdir(parents=True, exist_ok=True)
        fd, tmp_name = tempfile.mkstemp(prefix=name + ".", dir=self.root)
        try:
            with os.fdopen(fd, "wb") as out:
                out.write(data)
                out.flush()
                os.fsync(out.fileno())
            os.replace(tmp_name, self.root / name)
        except Exception:
            try:
                os.unlink(tmp_name)
            except OSError:
                pass
            raise
        print(f"RECEIVED {name} bytes={length}", flush=True)
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.end_headers()
        self.wfile.write(f"OK {name} {length}\n".encode())

    def log_message(self, fmt, *args):
        print("HTTP " + (fmt % args), flush=True)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, type=Path)
    ap.add_argument("--host", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=18081)
    args = ap.parse_args()
    Handler.root = args.root.resolve()
    server = HTTPServer((args.host, args.port), Handler)
    print(f"RUN106_RECEIVER_LISTENING={args.host}:{args.port} root={Handler.root}", flush=True)
    server.serve_forever()

if __name__ == "__main__":
    main()
