#cliente
import client_server
import socket

client_socket = None
servers = []
        
def client_sending(master,message_to_send):
    client_server.send_all(master,message_to_send)
    message_received = ""
    while message_received.pos("SUCCESS\n") > 0:
        message_received_buffer = b""
        message_received_buffer = master.recv()
        message_received_buffer.decode("utf-8")
        message_received += message_received_buffer

#realiza la coneccion con el servidor
def connect(address):
    serverIP, serverPort = address
    master = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    master.connect((serverIP, serverPort))
    master.settimeout(None)
    return master