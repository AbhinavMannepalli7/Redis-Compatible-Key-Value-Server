import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(("localhost", 4950))  # server's port

def send_cmd(cmd):
    sock.sendall(cmd.encode())          # send raw bytes, exactly what you write
    print(f"Sent: {cmd!r}")

def recv_data():
    response = sock.recv(4096)          # read whatever comes back
    print(f"Got:  {response!r}")

send_cmd("PING hi\r\n")
recv_data()
send_cmd("SET foo bar\r\nSET hello world\r\n")
recv_data()
send_cmd("GET f")
send_cmd("oo\r\n")
recv_data()
send_cmd("GET hello\r\n")
recv_data()
send_cmd("EXISTS hello\r\n")
recv_data()
send_cmd("DEL foo hello\r\n")
recv_data()

sock.close()
