from jsonrpc_redes import Server
import threading
import time
import sys
import math

def server():
    # Servidor metodos de geometria
    
    host, port = 'localhost', 8082
    
    def get_pi_value():
        return math.pi
    
    def circle_area_and_perimeter(radio):
        area = math.sqrt(radio)*math.pi
        perimeter = 2*radio*math.pi
        return area, perimeter
    
    def points_line_path_length(**kwargs):
        size = len(kwargs)
        length = 0
        if (size > 0):
            p = kwargs.get('p1')
            for i in range(2,size + 1):
                p_next = kwargs.get('p'+str(i))
                length += math.dist(p,p_next)
                p = p_next
        return length
        
    server = Server((host, port))
    server.add_method(get_pi_value, "get_pi")
    server.add_method(circle_area_and_perimeter)
    server.add_method(points_line_path_length, "path_length")
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