from jsonrpc_redes import Server
import threading
import time
import sys

def server():
    # Servidor metodos de strings
    
    host, port = 'localhost', 8081
    
    def concat_strings(*args):
        total = ''
        for arg in args:
            total += arg
        return total
    
    def repeat_string(string, cant):
        return str(string)*cant
    
    def truncate_string(string, pos):
        return string[:-pos]
        
    server = Server((host, port))
    server.add_method(concat_strings, 'concat_str')
    server.add_method(repeat_string)
    server.add_method(truncate_string)
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