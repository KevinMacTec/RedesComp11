#Cliente
from socket import *
import json

class Client:
    def __init__(self, address, port):
        self.address = address
        self.port = port
        self.next_id = 0

    def __getattr__(self, name):
        def rpc_trigger(*args, **kwargs):
            #print(f"Llamada a método remoto: {name} con argumentos: {args} y argumentos keyword: {kwargs}")
            notif = False
            if ('notify' in kwargs.keys() and kwargs['notify']):
                notif = True
                del kwargs['notify']
            if (args and not kwargs):
                params = list(args)
            elif (kwargs and not args):
                params = kwargs
            else:
                error = Exception()
                error.code = 0
                error.message = 'prueba'
                raise error
            msg = {"jsonrpc": "2.0", "method": name, "params": params }
            if not notif:
                msg["id"] = self.get_id()
            req_out = json.dumps(msg)
            client = socket(AF_INET, SOCK_STREAM)
            client.connect((self.address, self.port))
            client.sendall(req_out.encode())
            if not notif:
                res_in = client.recv(1024)
                rslt = json.loads(res_in.decode())
                if ('error' in rslt.keys()):
                    error = Exception()
                    error.code = rslt['error']['code']
                    error.message = rslt['error']['message']
                    raise error
                client.close()
                return rslt['result']
            else:
                client.close()
        return rpc_trigger
    
    def get_id(self):
        id = self.next_id
        self.next_id += 1
        return id 

def connect(adress, port):
    conn = Client(adress, port)
    return conn
