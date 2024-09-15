from socket import *
import types
import json
from .utils import valid_count

class Server:
    def __init__(self, tuple_host_port):
        # Inicializo el socket que escucha peticiones
        self.host_port = tuple_host_port
        self.methods = {}
        self.server_skt = socket(AF_INET, SOCK_STREAM)
        self.server_skt.bind(self.host_port)
        self.server_skt.listen(3)
    
    def add_method(self, method, *method_call_name):
        # Metodo para agregar metodos a la clase
        if isinstance(method, types.FunctionType):
            if method_call_name:
                key = method_call_name[0]
            else:
                key = method.__name__
            self.methods[key] = method
        else: 
            raise TypeError('method debe ser una funcion')
        
    def serve(self):
        while True:
            try:
                # Comienzo procedimiento para aceptar solicitud
                conn, addr = self.server_skt.accept()
                # Seteo timeout una vez acepto solicitud
                conn.settimeout(10)
                # Recibo hasta que todos los '{' abiertos se cierren con un '}'
                req_in = ''
                cant_open = 0
                cant_close = 0
                while True:
                    packet = conn.recv(1024).decode()
                    cant_open += valid_count(packet, '{')
                    cant_close += valid_count(packet, '}')
                    req_in += packet
                    print(req_in)
                    if cant_open != 0 and cant_open == cant_close:
                        break
                # Una vez recibido todo el request lo paso al handler 
                notif, rslt = self.__rpc_handler(req_in)
                if not notif:
                    res_out = json.dumps(rslt)
                    conn.sendall(res_out.encode())
            except socket.timeout as e:
            # Manejo timeout de solicitud
                print('Timeout esperando la solicitud del cliente')
            except Exception as e:
            # Manejo cualquier otra excepcion 
                print('Error en la conexión: ',e)
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
                    if (isinstance(result, tuple)):
                        result = list(result)
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
