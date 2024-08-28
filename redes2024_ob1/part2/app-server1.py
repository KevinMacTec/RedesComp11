from jsonrpc_redes import Server
import threading
import time
import sys

def server():
    # Este método es un ejemplo de cómo se puede usar el servidor.
    # Se inicia un servidor en el puerto 8080 y se añaden dos métodos
    
    host, port = 'localhost', 8080
    # host, port = '', 8080
    
    def substract(a, b):
        return a - b
        
    def divide(a, b):
        return a / b

    def concat_strings(*args):
        total = ''
        for arg in args:
            total += arg
        return total
    
    def repeat_string(string, cant):
        return str(string)*cant
        
    server = Server((host, port))
    server.add_method(substract)
    server.add_method(divide)
    server.add_method(concat_strings, 'concat_str')
    server.add_method(repeat_string)
    server_thread = threading.Thread(target=server.serve)
    server_thread.daemon = True
    server_thread.start()
    
    print ("Servidor ejecutando: %s:%s" % (host, port))
    
    try:
        while True:
            time.sleep(0.5)
    except KeyboardInterrupt:
        server.shutdown()
        print('Terminado.')
        sys.exit()
    
if __name__ == "__main__":
    server()