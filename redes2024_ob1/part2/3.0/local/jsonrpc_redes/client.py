#Cliente
from socket import *
import json
import uuid
from .utils import valid_count

class Client:
    def __init__(self, address, port):
        self.address = address
        self.port = port

    def __getattr__(self, name):
        def rpc_trigger(*args, **kwargs):
            print("Llamada a método remoto:",name,"con argumentos: ",args," y argumentos keyword: ",kwargs)
            notif = False
            # Chequeo si es notificacion
            if ('notify' in kwargs.keys() and kwargs['notify']):
                notif = True
                del kwargs['notify']
            # Chequeo modalidad de parametros y si no son ambas modalidades juntas
            if kwargs:
                params = kwargs
                # Mezcla args y kwargs
                if args:
                    error = Exception()
                    error.code = -32600
                    error.message = 'Invalid Request'
                    raise error
            else:
                params = list(args)
            # Armo el mensaje
            msg = {"jsonrpc": "2.0", "method": name, "params": params }
            if not notif:
                msg["id"] = str(uuid.uuid4())
            # Armo la request parseando a string
            req_out = json.dumps(msg)
            # Comienzo procedimiento de envio
            client = socket(AF_INET, SOCK_STREAM)
            # Seteo timeout de conexion
            client.settimeout(10)
            try:
                client.connect((self.address, self.port))
            except socket.timeout as e:
            # Manejo timeout de conexion
                error = Exception()
                error.code = -32601
                error.message = 'Timeout error: ' + str(e)
                raise error
            client.sendall(req_out.encode())
            # Fin procedimiento de envio, paso a esperar respuesta (si no es notificacion)
            if not notif:
                # Seteo timeout de respuesta
                client.settimeout(10)
                try:
                    # Recibo hasta que todos los '{' abiertos se cierren con un '}'
                    res_in = ''
                    cant_open = 0
                    cant_close = 0
                    while True:
                        packet = client.recv(4).decode()
                        cant_open += valid_count(packet, '{')
                        cant_close += valid_count(packet, '}')
                        res_in += packet
                        if cant_open != 0 and cant_open == cant_close:
                            break
                except socket.timeout as e:
                # Manejo timeout de respuesta
                    error = Exception()
                    error.code = -32601
                    error.message = 'Timeout error: ' + str(e)
                    raise error
                # Parseo json, si existe error con estructura salta aqui
                rslt = json.loads(res_in)
                if ('error' in rslt.keys()):
                    error = Exception()
                    error.code = rslt['error']['code']
                    error.message = rslt['error']['message']
                    raise error
                client.close()
                # Si todo correcto retorno el resultado
                return rslt['result']
            else:
                client.close()
        return rpc_trigger
    

def connect(address, port):
    conn = Client(address, port)
    return conn
