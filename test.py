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
    print(f"DEL response: {resp!r}") 

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


# 4. Concurrency: many clients sending shared keys, does server prevent race conditions?

def hammer_incr(key, n_ops, results, idx):
    s = connect()
    for _ in range(n_ops):
        send_cmd(s, f"INCR {key}")
    s.close()
    results[idx] = True


def test_concurrent_incr(n_threads=8, n_ops=200):
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
    # checking that the numbers got updated correctly, one operation at a time
    assert str(n_threads * n_ops) in resp, "LOST UPDATES — locking bug in Store"
    print("test_concurrent_incr: PASS")


# 5. Load / throughput

def load_worker(n_ops, results, idx):
    s = connect()
    latencies = []
    key = "".join(random.choices(string.ascii_lowercase, k=6))
    for i in range(n_ops):
        start = time.perf_counter()
        send_cmd(s, f"SET {key} val{i}")
        send_cmd(s, f"GET {key}")
        end = time.perf_counter()
        latencies.append((end-start)*1_000_000)
    results[idx] = latencies
    s.close()

def percentile(sorted_latencies, p):
    idx = int(p * len(sorted_latencies))
    return sorted_latencies[min(idx, len(sorted_latencies) - 1)]

def test_load(n_threads=16, n_ops=500):
    # the list of latencies for each operation in each thread
    results = [None] * n_threads
    threads = []
    start = time.perf_counter()
    for i in range(n_threads):
        t = threading.Thread(target=load_worker, args=(n_ops, results, i))
        threads.append(t)
        t.start()
    for t in threads:
        t.join()
    total_elapsed = time.perf_counter() - start
    all_latencies = [lat for thread_latencies in results for lat in thread_latencies]
    all_latencies.sort()

    total_ops = n_threads * n_ops * 2  # SET + GET per iteration
    ops_per_sec = total_ops / total_elapsed
    p50 = percentile(all_latencies, 0.50)
    p99 = percentile(all_latencies, 0.99)

    print(f"Load test ({n_threads} threads x {n_ops} ops):")
    print(f"  {total_ops} ops in {total_elapsed:.2f}s = {ops_per_sec:.0f} ops/sec")
    print(f"  p50 latency: {p50:.1f}µs")
    print(f"  p99 latency: {p99:.1f}µs")

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