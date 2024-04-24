import socket
import select

def forward_data(sock, remote):
    data = sock.recv(4096)
    if data:
        print(f"Forwarding: {len(data)} bytes")
        try:
            decoded_data = data.decode('utf-8', errors='backslashreplace')
            print(decoded_data)
        except UnicodeDecodeError:
            print("Error: Unable to decode data as UTF-8")
            print(data)
        remote.sendall(data)
    else:
        print("Closing connection")
        sock.close()
        remote.close()

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('0.0.0.0', 9999))  # Proxy server will run on port 9999
    server.listen(5)

    input_sockets = [server]
    clients = {}
    addresses = {}

    while True:
        readable, _, _ = select.select(input_sockets, [], [])
        for s in readable:
            if s is server:
                client_socket, client_address = server.accept()
                print(f"Connection from {client_address}")
                # Establish connection to IRC server
                server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                irc_server_address = ('irc.libera.chat', 6667)  # IRC server address and port
                server_socket.connect(irc_server_address)
                input_sockets.append(client_socket)
                input_sockets.append(server_socket)
                clients[client_socket] = server_socket
                clients[server_socket] = client_socket
                addresses[client_socket] = client_address
            else:
                try:
                    forward_data(s, clients[s])
                except ConnectionResetError:
                    print("Connection reset by peer")
                    for sock in [s, clients[s]]:
                        if sock in input_sockets:
                            input_sockets.remove(sock)
                        sock.close()
                        if sock in clients:
                            del clients[sock]
                        if clients[sock] in clients:
                            del clients[clients[sock]]
                except Exception as e:
                    print(f"Error: {e}")
                    for sock in [s, clients[s]]:
                        if sock in input_sockets:
                            input_sockets.remove(sock)
                        sock.close()
                        if sock in clients:
                            del clients[sock]
                        if clients[sock] in clients:
                            del clients[clients[sock]]

if __name__ == '__main__':
    main()