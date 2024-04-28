import socket
import select

def forward_data(sock, remote):
    try:
        data = sock.recv(4096)
        if data:
            print(f"Forwarding: {len(data)} bytes")
            try:
                decoded_data = data.decode('utf-8', errors='backslashreplace')
                print(decoded_data)
                if decoded_data.startswith('JOIN'):
                    # 클라이언트가 JOIN 명령을 보냈을 때 서버에서 응답 보내기
                    response = ':server.example.com 461 * JOIN :Not enough parameters\r\n'
                    remote.sendall(response.encode('utf-8'))
                else:
                    remote.sendall(data)
            except UnicodeDecodeError:
                print("Error: Unable to decode data as UTF-8")
                print(data)
                remote.sendall(data)
        else:
            print("Closing connection")
            raise ConnectionResetError
    except (ConnectionResetError, OSError):
        print("Connection reset by peer or socket error")
        sock.close()
        remote.close()

def remove_socket(sock, input_sockets, clients):
    if sock in input_sockets:
        input_sockets.remove(sock)
    if sock in clients:
        del clients[sock]
    if sock in clients.values():
        for client, server in clients.items():
            if server == sock:
                del clients[client]
                break

def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('0.0.0.0', 9999))  # Proxy server will run on port 9999
    server.listen(5)

    input_sockets = [server]
    clients = {}
    addresses = {}

    while True:
        try:
            readable, _, _ = select.select(input_sockets, [], [])
            for s in readable:
                if s is server:
                    client_socket, client_address = server.accept()
                    print(f"Connection from {client_address}")
                    # Establish connection to IRC server
                    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    irc_server_address = ("localhost", 8083)  # IRC server address and port
                    server_socket.connect(irc_server_address)

                    # 클라이언트에게 초기 환영 메시지 보내기
                    welcome_message = ':server.example.com 001 nickname :Welcome to the Internet Relay Chat Network, nickname\r\n'
                    client_socket.sendall(welcome_message.encode('utf-8'))

                    input_sockets.append(client_socket)
                    input_sockets.append(server_socket)
                    clients[client_socket] = server_socket
                    clients[server_socket] = client_socket
                    addresses[client_socket] = client_address
                else:
                    try:
                        forward_data(s, clients[s])
                    except ConnectionResetError:
                        remove_socket(s, input_sockets, clients)
                        remove_socket(clients[s], input_sockets, clients)
                    except KeyError:
                        remove_socket(s, input_sockets, clients)
                    except Exception as e:
                        print(f"Error: {e}")
                        remove_socket(s, input_sockets, clients)
                        remove_socket(clients[s], input_sockets, clients)
        except ValueError:
            # 유효하지 않은 파일 디스크립터 처리
            for sock in input_sockets:
                try:
                    if sock.fileno() == -1:
                        remove_socket(sock, input_sockets, clients)
                except socket.error:
                    remove_socket(sock, input_sockets, clients)

if __name__ == '__main__':
    main()