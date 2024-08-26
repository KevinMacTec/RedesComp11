#servidor
import client_server
import threading
import socket

class Server:
    def __init__(self, host, port):
        self._host = host
        self._port = port
        self._methods = {}
        self._clients = []
        self._server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._server_socket.bind((self.host, self.port))
        self._server_socket.listen(1)
        print(f"Servidor iniciado en {self.host}:{self.port}")

    def print(self):
        print("Servidor ", self.host,":",self.port)

    def add_method(self, *args):
        if not args:
            raise ValueError("Debe proporcionar al menos el nombre del metodo.")
        
        function_name = args[0]
        parameters = args[1:]
        self.methods[function_name] = parameters

    def shutdown(self):
        self.server_socket.close()
        self.print()
        print(", cerro.")

    def serve(self):
        while True:
            try:
                client_socket, client_address = self._server_socket.accept()
                print("Conexión establecida con ", client_address)
                thread = threading.Thread(target=self.accept_connection, args=client_socket)
                thread.start()
            except Exception as err:
                print("Error al conectar ")
                self.print()
                print(" con ")
                print(client_address)

    def accept_connection(self, client_socket):
        try:
            #hay que ver como se comunica entre servidor y cliente
            conect = False
        except Exception:
            client_socket.close()

    def server_sending(client,message_to_send):
        client_server.send_all(client,message_to_send)

    def connect(self,skt):
        while True:
            try:
                client, _ = skt.accept()
                thread = threading.Thread(target=self.accept_connection, args=client)
                thread.start()
            except Exception:
                print("Error al conectar con TCP")


        