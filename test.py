import socket

with open("input.txt", "rb") as f:
    data = f.read()

s = socket.create_connection(("localhost", 4950))
s.sendall(data)
s.shutdown(socket.SHUT_WR)  # half-close: done sending, still reading

received = b""
while True:
    chunk = s.recv(4096)
    if not chunk:
        break
    received += chunk

print(f"sent {data.count(chr(10).encode())} lines, got back {received.count(chr(10).encode())} lines")
