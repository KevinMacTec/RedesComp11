from socket import *
import types
import json

class Server:
    def __init__(self, tuple_host_port):
        self.host_port = tuple_host_port
        self.methods = {}
        self.server_skt = socket(AF_INET, SOCK_STREAM)
        self.server_skt.bind(self.host_port)
        self.server_skt.listen(1)

    def print(self):
        print("Servidor ", self.host_port[0],":",self.host_port[1])
    
    def add_method(self, method, *method_call_name):
        if isinstance(method, types.FunctionType):
            if method_call_name:
                key = method_call_name[0]
            else:
                key = method.__name__
            self.methods[key] = method
        else: 
            raise TypeError('method debe ser una funcion')
        
    def serve(self):
        while(True):
            conn, addr = self.server_skt.accept()
            req_in = conn.recv(1024).decode()
            msg = json.loads(req_in)
            notif, rslt = self.__rpc_handler(msg)
            if (not notif):
                res_out = json.dumps(rslt)
                conn.send(res_out.encode())
            conn.close()
    
    def shutdown(self):
        self.server_skt.close()
        self.print()
        print(", cerro.")

    def __rpc_handler(self, msg):
        print(f'Mensaje: {msg}')
        if ('id' in msg.keys()):
            method = msg.get("method")
            params = msg.get("params")
            req_id = msg.get("id")
            print('Tipo :', type(params), params)
            if isinstance(params, dict):
                params = list(params.values())  # Convertir los valores del diccionario en una lista
            print('*params :', *params)
            if method in self.methods:
                #-32700	Parse error	Invalid JSON was received by the server.
                # -32600	Invalid Request	The JSON sent is not a valid Request object.
                # -32602	Invalid params	Invalid method parameter(s).
                # -32000 to -32099
                try:
                    result = self.methods[method](*params)
                    response = {
                        "jsonrpc": "2.0",
                        "result": result,
                        "id": req_id
                    }
                except Exception as e:
                    response = {
                        "jsonrpc": "2.0",
                        "error": {"code": -32603, "message": str(e)},
                        "id": req_id
                    }
            else:
                response = {
                    "jsonrpc": "2.0",
                    "error": {"code": -32601, "message": "Method not found"},
                    "id": req_id
                }
            print('Respuesta: ', response)
            return (False, response)
        else:
            return (True, None)



    
  