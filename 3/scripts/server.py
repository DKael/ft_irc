from dataclasses import dataclass
import re
import select
import socket
import sys
from termcolor import colored

IP = r"(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)"

BACKLOG = 32
BUFFER_SIZE = 8192
MAX_PORT = 65535


@dataclass
class Connection:
    idx: int
    client_socket: socket
    server_socket: socket


def print_info(message):
    print(colored(message, "blue"))


def find_connection(connections, socket):
    for connection in connections:
        if socket == connection.client_socket or socket == connection.server_socket:
            return connection


def parse_address(address, name):
    if address.count(":") != 1:
        print(f"Error: invalid {name} address")
        return None
    host, port = address.split(":")
    if not re.fullmatch(rf"{IP}.{IP}.{IP}.{IP}", host):
        print(f"Error: invalid {name} host")
        return None
    if not re.fullmatch(r"\d{4,5}", port) or (port := int(port)) > MAX_PORT:
        print(f"Error: invalid {name} port")
        return None
    return host, int(port)


def parse_argv():
    if len(sys.argv) != 3:
        print("Error: wrong number of arguments provided.")
    elif (proxy_address := parse_address(sys.argv[1], "proxy")) and (
        server_address := parse_address(sys.argv[2], "server")
    ):
        return proxy_address, server_address
    print(f"Usage: python3 {sys.argv[0]} proxy_host:proxy_port server_host:server_port")
    print(f"Example: python3 {sys.argv[0]} 127.0.0.1:5555 188.240.145.40:6667")
    sys.exit(1)


proxy_address, server_address = parse_argv()
proxy_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
proxy_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
proxy_socket.bind(proxy_address)
proxy_socket.listen(BACKLOG)
print_info(f"Listening on {sys.argv[1]}.")
print_info(f"Forwarding to {sys.argv[2]}.")
total_connections = 0
connections = []
while True:
    sockets = [
        proxy_socket,
        *[c.client_socket for c in connections],
        *[c.server_socket for c in connections],
    ]
    responses = select.select(sockets, [], [])[0]
    for sender in responses:
        if sender == proxy_socket:
            client_socket, _ = proxy_socket.accept()
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.connect(server_address)
            total_connections += 1
            connection = Connection(total_connections, client_socket, server_socket)
            connections.append(connection)
            print_info(f"Client {total_connections} connected.")
        else:
            data = sender.recv(BUFFER_SIZE)
            connection = find_connection(connections, sender)
            if sender == connection.client_socket:
                direction = f"Client {connection.idx} to Server:"
                color = "red"
                receiver = connection.server_socket
                name = "Client"
            else:
                direction = f"Server to Client {connection.idx}:"
                color = "green"
                receiver = connection.client_socket
                name = "Server"
            if data:
                s = "".join(
                    chr(c) for c in data if c == 10 or c == 13 or 32 <= c <= 126
                ).strip()
                if "PING " not in s and "PONG " not in s:
                    print(colored(direction, color))
                    print(colored(s, color))
                receiver.sendall(data)
            else:
                print_info(f"{name} {connection.idx} disconnected.")
                sender.close()
                receiver.close()
                connections.remove(connection)