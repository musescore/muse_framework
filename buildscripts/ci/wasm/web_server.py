# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2026 MuseScore Limited and others
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# A local static server for the web (wasm) version of an app based on the Muse framework.
#
#   python3 web_server.py --dir <directory> [--port 8081] [--page viewer.html] [--open]
#
# It serves the build artifacts without caching and prints the logs of the app
# (the browser console) to the terminal: a small script is injected into the served
# app page, it forwards the console messages to POST /log.

import argparse
import http.server
import json
import os
import re
import sys
import threading
import webbrowser

LOG_ENDPOINT = "/log"
# The page that loads the app, the console forwarding is injected into it
APP_PAGE = "/viewer.html"

# Forwards console messages, errors and unhandled rejections to the server, in batches
FORWARD_SCRIPT = """
<script>
(function () {
    const queue = [];
    let timer = null;

    function toText(arg) {
        if (typeof arg === "string") return arg;
        if (arg instanceof Error) return arg.stack || String(arg);
        try { return JSON.stringify(arg); } catch (e) { return String(arg); }
    }

    // The total body size of the keepalive requests is limited to 64 KiB
    const KEEPALIVE_LIMIT = 60 * 1024;
    const encoder = new TextEncoder();

    function send(batch, keepalive) {
        fetch("%s", { method: "POST", body: JSON.stringify(batch), keepalive: keepalive })
            .catch(function () { Array.prototype.unshift.apply(queue, batch); });
    }

    function flush() {
        timer = null;
        if (queue.length) send(queue.splice(0, queue.length), false);
    }

    // The page is being unloaded: only a keepalive request is delivered, so send what fits into its limit
    function flushOnPageHide() {
        clearTimeout(timer);
        timer = null;
        let size = 2;
        let count = 0;
        while (count < queue.length) {
            const msg = queue[count];
            const msgSize = encoder.encode(JSON.stringify(msg)).length + 1;
            if (size + msgSize > KEEPALIVE_LIMIT) {
                if (count === 0) {
                    msg.text = msg.text.slice(0, Math.floor(msg.text.length / 2)) + " ...";
                    continue;
                }
                break;
            }
            size += msgSize;
            ++count;
        }
        if (count) send(queue.splice(0, count), true);
    }

    function push(level, args) {
        queue.push({ level: level, text: Array.prototype.map.call(args, toText).join(" ") });
        if (!timer) timer = setTimeout(flush, 100);
    }

    ["log", "info", "warn", "error", "debug"].forEach(function (level) {
        const orig = console[level].bind(console);
        console[level] = function () { push(level, arguments); orig.apply(null, arguments); };
    });

    window.addEventListener("error", function (e) {
        push("error", ["Uncaught", e.error || e.message]);
    });
    window.addEventListener("unhandledrejection", function (e) {
        push("error", ["Unhandled rejection", e.reason]);
    });
    window.addEventListener("pagehide", flushOnPageHide);
})();
</script>
""" % LOG_ENDPOINT


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {**http.server.SimpleHTTPRequestHandler.extensions_map,
                      ".wasm": "application/wasm", ".js": "application/javascript"}

    def end_headers(self):
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        super().end_headers()

    def do_GET(self):
        path = self.path.split("?", 1)[0]
        if path == APP_PAGE and self._serve_with_forwarding(path):
            return
        super().do_GET()

    def do_POST(self):
        if self.path != LOG_ENDPOINT:
            self.send_error(404)
            return

        length = int(self.headers.get("Content-Length", 0))
        try:
            messages = json.loads(self.rfile.read(length) or b"[]")
        except ValueError:
            messages = []

        for msg in messages:
            level = str(msg.get("level", "log")).upper()
            print(f"[app] {level:5} | {msg.get('text', '')}", flush=True)

        self.send_response(204)
        self.end_headers()

    def _serve_with_forwarding(self, path):
        file = os.path.join(self.directory, path.lstrip("/"))
        try:
            with open(file, "rb") as f:
                html = f.read().decode("utf-8")
        except OSError:
            return False

        # Before any other script, so that all the messages are forwarded:
        # right after <head ...>, or else before the first <script>
        head = re.search(r"<head\b[^>]*>", html, re.IGNORECASE)
        if head:
            pos = head.end()
        else:
            script = re.search(r"<script\b", html, re.IGNORECASE)
            pos = script.start() if script else -1

        if pos >= 0:
            html = html[:pos] + FORWARD_SCRIPT + html[pos:]
        else:
            print(f"[server] ERROR | no <head> or <script> in {path}, the app logs are not forwarded",
                  file=sys.stderr, flush=True)
        data = html.encode("utf-8")

        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)
        return True

    def log_message(self, format, *args):
        # The log forwarding requests are not interesting
        if self.command == "POST" and self.path == LOG_ENDPOINT:
            return
        sys.stderr.write("[http] %s\n" % (format % args))


def main():
    parser = argparse.ArgumentParser(description="Serves the web version of an app and prints its logs")
    parser.add_argument("--dir", default=os.getcwd(), help="directory to serve (the build artifacts)")
    parser.add_argument("--port", type=int, default=8081)
    parser.add_argument("--page", default="viewer.html", help="the page that loads the app")
    parser.add_argument("--open", action="store_true", help="open the page in the browser")
    args = parser.parse_args()

    global APP_PAGE
    APP_PAGE = "/" + args.page.lstrip("/")

    def handler(*handler_args, **kwargs):
        return Handler(*handler_args, directory=args.dir, **kwargs)

    server = http.server.ThreadingHTTPServer(("127.0.0.1", args.port), handler)

    url = f"http://127.0.0.1:{args.port}{APP_PAGE}"
    print(f"=== Serve {args.dir} at {url} ===", flush=True)
    print("NOTE: click on the page once to start audio", flush=True)

    # The server is already listening here
    if args.open:
        threading.Timer(0.5, webbrowser.open, [url]).start()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
