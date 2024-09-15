from jsonrpc_redes import Server
import threading
import time
import sys

def server():
    # Servidor métodos de numeros
    
    host, port = '200.0.0.10', 8080
    
    def substract(a, *b):
        return a - sum(list(b))
        
    def divide(a, b):
        return a / b

    def power_of(base , exp):
        return pow(base, exp)

    def n_root(base, n):
        return pow(base, 1/n)
        
    server = Server((host, port))
    server.add_method(substract)
    server.add_method(divide)
    server.add_method(power_of, "pow")
    server.add_method(n_root, "nrt")
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