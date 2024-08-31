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
            try:
                conn, addr = self.server_skt.accept()
                req_in = self._receive_data(conn)
                notif, rslt = self.__rpc_handler(req_in)
                if not notif:
                    res_out = json.dumps(rslt)
                    conn.send(res_out.encode())
            except Exception as e:
                print("Error en la conexión: ",e)
            finally:
                conn.close()
    
    def shutdown(self):
        self.server_skt.close()
        print("Servidor ",self.host_port[0], ":",self.host_port[1]," apagado.")

    def __rpc_handler(self, req):
        # Control JSON es parseable
        try:
            msg = json.loads(req)
        except Exception:
            response = {
                    "jsonrpc": "2.0",
                    "error": {"code": -32700, "message": "Parse error"},
                    "id": None
                }
            print("Respuesta: ", response)
            return (False, response)
        print("Solicitud: ",msg)
        if ('id' in msg.keys()):
        # Control estructura válida (solo si no es notificacion)
            try:
                if msg.get("jsonrpc") != "2.0":
                    raise Exception()
                method = msg.get("method")
                if not isinstance(method, str):
                    raise Exception()
                params = msg.get("params")
                if not(isinstance(params, list) or isinstance(params, dict)):
                    raise Exception()
                req_id = msg.get("id")
            except Exception:
                response = {
                    "jsonrpc": "2.0",
                    "error": {"code": -32600, "message": "Invalid Request"},
                    "id": req_id
                }
                print("Respuesta: ", response)
                return (False, response)
            # Request válido
            # Control existe método
            if method in self.methods:
                # Control ejecucion correcta del metodo
                try:
                    if isinstance(params, dict):
                        result = self.methods[method](**params)
                    else:
                        result = self.methods[method](*params)
                    response = {
                        "jsonrpc": "2.0",
                        "result": result,
                        "id": req_id
                    }
                except Exception as e:
                    if isinstance(e, TypeError):
                        response = {
                            "jsonrpc": "2.0",
                            "error": {"code": -32602, "message": "Invalid params"},
                            "id": req_id
                        }
                    else:
                        response = {
                            "jsonrpc": "2.0",
                            "error": {"code": -32603, "message": "Internal error"},
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



    
  