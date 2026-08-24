import socket
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(("localhost", 4950))  # server's port

def send_cmd(cmd):
    sock.sendall(cmd.encode())          # send raw bytes, exactly what you write
    print(f"Sent: {cmd!r}")

def recv_data():
    response = sock.recv(4096)          # read whatever comes back
    print(f"Got:  {response!r}")

send_cmd("SET foo bar\r\n")
recv_data()

send_cmd("EXPIRE foo 5\r\n")
recv_data()

time.sleep(2)

send_cmd("EXPIRE foo 20\r\n")
recv_data()

time.sleep(6)

send_cmd("GET foo\r\n")
recv_data()

sock.close()
