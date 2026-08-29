import socket
import threading
import time
import random
import string

HOST = "127.0.0.1"
PORT = 4950

def connect():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    return s


def send_cmd(sock, line: str, bufsize=4096) -> str:
    sock.sendall((line + "\r\n").encode())
    return sock.recv(bufsize).decode(errors="replace")


# 1. Basic correctness

def test_basic_set_get():
    s = connect()
    resp = send_cmd(s, "SET foo bar")
    assert "OK" in resp, f"unexpected SET response: {resp!r}"

    resp = send_cmd(s, "GET foo")
    assert "bar" in resp, f"unexpected GET response: {resp!r}"

    resp = send_cmd(s, "GET missing_key_xyz")
    assert "-1" in resp or "nil" in resp.lower(), f"unexpected nil response: {resp!r}"

    s.close()
    print("test_basic_set_get: PASS")


def test_del():
    s = connect()
    send_cmd(s, "SET todelete 123")
    resp = send_cmd(s, "DEL todelete")
    print(f"DEL response: {resp!r}")  # check this matches your actual format

    resp = send_cmd(s, "GET todelete")
    assert "-1" in resp or "nil" in resp.lower(), f"key should be gone: {resp!r}"
    s.close()
    print("test_del: PASS")


# 2. Expiry (TTL)

def test_expiry():
    s = connect()
    send_cmd(s, "SET expiring hello")
    resp = send_cmd(s, "EXPIRE expiring 1")  # 1 second TTL
    print(f"EXPIRE response: {resp!r}")

    resp = send_cmd(s, "GET expiring")
    assert "hello" in resp, "key should still exist immediately after EXPIRE"

    time.sleep(1.5)  # wait past expiry

    resp = send_cmd(s, "GET expiring")
    assert "-1" in resp or "nil" in resp.lower(), f"key should have expired: {resp!r}"
    s.close()
    print("test_expiry: PASS")


# 3. Partial reads / message split across packets

def test_partial_send():
    """Send a command byte-by-byte to make sure your buffering handles
    a command arriving across multiple read() calls."""
    s = connect()
    cmd = "SET slowkey slowvalue\r\n"
    for ch in cmd:
        s.sendall(ch.encode())
        time.sleep(0.01)  # force separate TCP packets/reads
    resp = s.recv(4096).decode()
    assert "OK" in resp, f"partial-send SET failed: {resp!r}"
    s.close()
    print("test_partial_send: PASS")


# 4. Concurrency: many clients hammering shared keys

def hammer_incr(key, n_ops, results, idx):
    s = connect()
    for _ in range(n_ops):
        send_cmd(s, f"INCR {key}")
    s.close()
    results[idx] = True


def test_concurrent_incr(n_threads=8, n_ops=200):
    """
    Since your Store is a single shared instance behind a mutex,
    N threads each doing INCR on the same key should sum correctly
    with no lost updates. This is the real test of your Day 10 locking.
    """
    s = connect()
    send_cmd(s, "SET counter 0")
    s.close()

    results = [False] * n_threads
    threads = []
    for i in range(n_threads):
        t = threading.Thread(target=hammer_incr, args=("counter", n_ops, results, i))
        threads.append(t)
        t.start()
    for t in threads:
        t.join()

    assert all(results), "one or more client threads failed"

    s = connect()
    resp = send_cmd(s, "GET counter")
    s.close()
    print(f"Final counter value: {resp!r} (expected {n_threads * n_ops})")
    assert str(n_threads * n_ops) in resp, "LOST UPDATES — locking bug in Store"
    print("test_concurrent_incr: PASS")


# 5. Load / throughput

def load_worker(n_ops, results, idx):
    s = connect()
    start = time.time()
    key = "".join(random.choices(string.ascii_lowercase, k=6))
    for i in range(n_ops):
        send_cmd(s, f"SET {key} val{i}")
        send_cmd(s, f"GET {key}")
    elapsed = time.time() - start
    results[idx] = elapsed
    s.close()


def test_load(n_threads=16, n_ops=500):
    results = [0.0] * n_threads
    threads = []
    start = time.time()
    for i in range(n_threads):
        t = threading.Thread(target=load_worker, args=(n_ops, results, i))
        threads.append(t)
        t.start()
    for t in threads:
        t.join()
    total_elapsed = time.time() - start
    total_ops = n_threads * n_ops * 2  # SET + GET per op
    print(f"Load test: {total_ops} ops in {total_elapsed:.2f}s "
          f"= {total_ops / total_elapsed:.0f} ops/sec")


if __name__ == "__main__":
    tests = [
        test_basic_set_get,
        test_del,
        test_expiry,
        test_partial_send,
        test_concurrent_incr,
        test_load,
    ]
    for test in tests:
        try:
            test()
        except AssertionError as e:
            print(f"{test.__name__}: FAIL — {e}")
        except (ConnectionRefusedError, OSError) as e:
            print(f"{test.__name__}: could not connect — is the server running? ({e})")
            break
        print()