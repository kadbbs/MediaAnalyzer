#!/usr/bin/env python3

from email.parser import BytesParser
from email.policy import default
import json
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
WEB_ROOT = ROOT / "web" / "static"
CORE_BIN = ROOT / "build" / "media-analyzer-core"
MAX_PROBE_BYTES = 1024 * 1024


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WEB_ROOT), **kwargs)

    def do_POST(self):
        if self.path != "/api/analyze":
            self.send_error(404, "not found")
            return
        try:
            response = self.handle_analyze()
            self.write_json(200, response)
        except Exception as exc:
            self.write_json(400, {"error": str(exc)})

    def handle_analyze(self):
        content_type = self.headers.get("Content-Type", "")
        if content_type.startswith("multipart/form-data"):
            return self.analyze_upload()
        return self.analyze_url()

    def analyze_upload(self):
        content_type = self.headers.get("Content-Type", "")
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(min(length, MAX_PROBE_BYTES + 65536))
        message = BytesParser(policy=default).parsebytes(
            b"Content-Type: " + content_type.encode("utf-8") + b"\r\n\r\n" + body
        )

        file_part = None
        for part in message.iter_parts():
            if part.get_param("name", header="content-disposition") == "file":
                file_part = part
                break

        if file_part is None or not file_part.get_filename():
            raise ValueError("missing uploaded file")
        filename = file_part.get_filename()
        data = file_part.get_payload(decode=True)[:MAX_PROBE_BYTES]
        detection = run_core(data, filename)
        return {
            "input": {
                "type": "file",
                "name": filename,
                "size": len(data),
            },
            "detection": detection,
        }

    def analyze_url(self):
        length = int(self.headers.get("Content-Length", "0"))
        payload = json.loads(self.rfile.read(min(length, 1024 * 1024)) or b"{}")
        target = payload.get("url", "")
        if not target.startswith(("http://", "https://")):
            raise ValueError("url must be http or https")

        request = urllib.request.Request(target, headers={"Range": f"bytes=0-{MAX_PROBE_BYTES - 1}"})
        try:
            with urllib.request.urlopen(request, timeout=20) as response:
                data = response.read(MAX_PROBE_BYTES)
                size = response.headers.get("Content-Length")
        except urllib.error.URLError as exc:
            raise ValueError(f"failed to fetch url: {exc}") from exc

        detection = run_core(data, target)
        return {
            "input": {
                "type": "url",
                "url": target,
                "size": int(size) if size and size.isdigit() else len(data),
            },
            "detection": detection,
        }

    def write_json(self, status, payload):
        body = json.dumps(payload, ensure_ascii=False, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def ensure_core():
    if CORE_BIN.exists():
        return
    subprocess.run(["cmake", "-S", str(ROOT), "-B", str(ROOT / "build")], check=True)
    subprocess.run(["cmake", "--build", str(ROOT / "build")], check=True)


def run_core(data: bytes, name_hint: str):
    ensure_core()
    suffix = Path(name_hint).suffix
    with tempfile.NamedTemporaryFile(suffix=suffix) as tmp:
        tmp.write(data)
        tmp.flush()
        result = subprocess.run(
            [str(CORE_BIN), tmp.name],
            check=True,
            text=True,
            capture_output=True,
        )
    return json.loads(result.stdout)


def main():
    port = 8080
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    ensure_core()
    server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    print(f"MediaAnalyzer dev server: http://127.0.0.1:{port}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
