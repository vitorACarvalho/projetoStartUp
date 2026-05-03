import os
import socket
import subprocess
import time
from typing import Optional

CUR_DIR = os.path.dirname(os.path.abspath(__file__))
SOFTWARE_FILENAME = os.path.join(CUR_DIR, "x64", "Debug", "udptestutility.exe")
CONFIG_FILENAME = os.path.join(CUR_DIR, "test_config.ini")


# ============================================================
#  CONFIG FILE GENERATOR
# ============================================================

def create_config_file(
    file_path: str,
    payload_length: int = 0,
    payload: str = "",
    destination_ip: str = "",
    destination_port: int = 0,
    sending_period: int = 0
) -> None:
    """Creates a configuration .ini file for udptestutility."""
    with open(file_path, "w", encoding="utf-8") as f:
        f.write("[Connection]\n")
        f.write("#name=test_connection\n")

        if payload_length > 0:
            f.write(f"payload_length={payload_length}\n")

        if payload:
            f.write(f"payload={payload}\n")

        if destination_ip:
            f.write(f"destination_ip={destination_ip}\n")

        if destination_port > 0:
            f.write(f"destination_port={destination_port}\n")

        if sending_period > 0:
            f.write(f"sending_period={sending_period}\n")


# ============================================================
#  UTILS
# ============================================================

def start_sender(config_file: str) -> subprocess.Popen:
    """Starts the udptestutility executable."""
    return subprocess.Popen(
        [SOFTWARE_FILENAME, config_file],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )


def cleanup(proc: subprocess.Popen, sock: socket.socket) -> None:
    """Ensures proper subprocess termination and resource cleanup."""
    proc.kill()
    proc.wait(timeout=1)
    sock.close()

    if os.path.exists(CONFIG_FILENAME):
        os.remove(CONFIG_FILENAME)


# ============================================================
#  TESTS
# ============================================================

def test_1() -> None:
    """Tests basic payload_length functionality."""
    payload_length = 128
    destination_ip = "127.0.0.1"
    destination_port = 5000
    sending_period = 1000

    create_config_file(
        CONFIG_FILENAME,
        payload_length=payload_length,
        destination_ip=destination_ip,
        destination_port=destination_port,
        sending_period=sending_period
    )

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((destination_ip, destination_port))

    proc = start_sender(CONFIG_FILENAME)

    try:
        while True:
            data, _ = sock.recvfrom(1024)
            if data:
                assert len(data) == payload_length, f"Payload size different than expected! Received={len(data)}, Expected={payload_length}"
                break
    finally:
        cleanup(proc, sock)


def test_2() -> None:
    """Tests payload content correctness."""
    payload = "abcdefghijklmnopqrstuvwxyz"
    destination_ip = "127.0.0.1"
    destination_port = 1234
    sending_period = 1000

    create_config_file(
        CONFIG_FILENAME,
        payload=payload,
        destination_ip=destination_ip,
        destination_port=destination_port,
        sending_period=sending_period
    )

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((destination_ip, destination_port))

    proc = start_sender(CONFIG_FILENAME)

    try:
        while True:
            data, _ = sock.recvfrom(1024)
            if data:
                assert data.decode() == payload, f"Received payload different than expected! Received={data}, Expected={payload}"
                break
    finally:
        cleanup(proc, sock)


def test_3() -> None:
    """
    Test 3 — Receives multicast packets and measures the average interval between transmissions.
    Checks whether the measured interval is within a ±10% tolerance.
    """

    payload_length = 64
    destination_ip = "239.0.0.99"
    destination_port = 6000
    sending_period_ms = 123  # expected interval in ms (not rate in Hz)

    create_config_file(
        CONFIG_FILENAME,
        payload_length=payload_length,
        destination_ip=destination_ip,
        destination_port=destination_port,
        sending_period=sending_period_ms
    )

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", destination_port))

    # Multicast join
    mreq = socket.inet_aton(destination_ip) + socket.inet_aton("0.0.0.0")
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

    proc = start_sender(CONFIG_FILENAME)

    try:
        deltas = []
        last_time: Optional[float] = None

        while len(deltas) < 10:
            data, _ = sock.recvfrom(1024)
            assert len(data) == payload_length

            now = time.time()
            if last_time is not None:
                delta_ms = (now - last_time) * 1000
                deltas.append(delta_ms)

            last_time = now

        avg_interval = sum(deltas) / len(deltas)

        tolerance = sending_period_ms * 0.10
        low, high = sending_period_ms - tolerance, sending_period_ms + tolerance

        assert low <= avg_interval <= high, \
            f"Sending period out of tolerance! Measured={avg_interval}, Expected={sending_period_ms}"

    finally:
        cleanup(proc, sock)


# ============================================================
#  RUN (example)
# ============================================================

if __name__ == "__main__":
    print("Running tests...")
    test_1()
    test_2()
    test_3()
    print("Tests completed successfully.")
    input("Press to exit")
