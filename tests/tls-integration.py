#!/usr/bin/env python3
"""Exercise TLS against a disposable local Valkey/Redis process."""
import os
from pathlib import Path
import socket
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent
TCLSH = os.environ.get("TCLSH", "tclsh")
SERVER = os.environ.get("VALKEY_SERVER", "valkey-server")
SERVICE_ENV = dict(os.environ)
SERVICE_ENV.pop("LD_LIBRARY_PATH", None)


def stop(process):
    process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


with tempfile.TemporaryDirectory(prefix="valkey-tls-") as temporary:
    directory = Path(temporary)
    with (directory / "openssl.log").open("w") as log:
        def openssl(*args):
            subprocess.run(["openssl", *args], cwd=directory, stdout=log,
                           stderr=log, check=True, env=SERVICE_ENV)

        for name in ("ca", "other-ca"):
            openssl("req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
                    "-subj", f"/CN={name}", "-keyout", f"{name}.key", "-out", f"{name}.pem")
        openssl("req", "-newkey", "rsa:2048", "-nodes", "-subj", "/CN=localhost",
                "-keyout", "server.key", "-out", "server.csr")
        (directory / "server.ext").write_text("subjectAltName=DNS:localhost,IP:127.0.0.1\n")
        openssl("x509", "-req", "-in", "server.csr", "-CA", "ca.pem", "-CAkey", "ca.key",
                "-CAcreateserial", "-days", "1", "-extfile", "server.ext", "-out", "server.pem")

    (directory / "users.acl").write_text(
        "user default off\nuser appuser on >tls-test-password ~* +@all\n")
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    with (directory / "server.log").open("w+") as log:
        server = subprocess.Popen([
            SERVER, "--bind", "127.0.0.1", "127.0.0.2", "--port", "0", "--tls-port", str(port),
            "--tls-cert-file", str(directory / "server.pem"),
            "--tls-key-file", str(directory / "server.key"),
            "--tls-ca-cert-file", str(directory / "ca.pem"), "--tls-auth-clients", "no",
            "--aclfile", str(directory / "users.acl"), "--save", "", "--appendonly", "no",
        ], stdout=log, stderr=subprocess.STDOUT, env=SERVICE_ENV)
        try:
            for _ in range(100):
                if server.poll() is not None:
                    log.seek(0)
                    raise RuntimeError(log.read())
                try:
                    with socket.create_connection(("127.0.0.1", port), timeout=0.1):
                        break
                except OSError:
                    time.sleep(0.05)
            else:
                raise RuntimeError("TLS server did not start")
            subprocess.run([TCLSH, str(ROOT / "tls-integration.tcl"), str(port), temporary],
                           check=True, timeout=30)

            # Keep an authenticated connection alive while replacing the TLS
            # server with a plaintext listener. The retry may send only TLS.
            script = ROOT / "tls-downgrade.tcl"
            client = subprocess.Popen([TCLSH, str(script), str(port), str(directory / "ca.pem")],
                                      stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE, text=True)
            try:
                assert client.stdout.readline().strip() == "READY"
                stop(server)
                with socket.socket() as listener:
                    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                    listener.bind(("127.0.0.1", port))
                    listener.listen()
                    listener.settimeout(5)
                    client.stdin.write("retry\n")
                    client.stdin.flush()
                    peer, _ = listener.accept()
                    with peer:
                        peer.settimeout(5)
                        data = peer.recv(8192)
                        assert data.startswith(b"\x16\x03"), data
                        assert b"tls-test-password" not in data and b"AUTH" not in data
                        peer.sendall(b"-ERR plaintext server\r\n")
                stdout, stderr = client.communicate(timeout=5)
                assert client.returncode == 0 and "REJECTED" in stdout, (stdout, stderr)
                print("Reconnect to plaintext server rejected without sending credentials")
            finally:
                if client.poll() is None:
                    stop(client)
        finally:
            if server.poll() is None:
                stop(server)
