#Cliente
from socket import *
import json
import uuid

class Client:
    def __init__(self, address, port):
        self.address = address
        self.port = port

    def __getattr__(self, name):
        def rpc_trigger(*args, **kwargs):
            print("Llamada a método remoto:",name,"con argumentos: ",args," y argumentos keyword: ",kwargs)
            notif = False
            if ('notify' in kwargs.keys() and kwargs['notify']):
                notif = True
                del kwargs['notify']
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
            msg = {"jsonrpc": "2.0", "method": name, "params": params }
            if not notif:
                msg["id"] = str(uuid.uuid4())
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
    

def connect(address, port):
    conn = Client(address, port)
    return conn
