import socket
import time

#funciones de cliente y servidor (algunas no se usan en la parte 2 ya que solo se implementa la coneccion y que se pasen mensajes)
#esto ya es parte de la implementacion del cliente y servidor (por ahora no va, probablemente para el sig lab pueda ir)

skt = socket
buff = ""
tiempo = 10 #tiempo maximo para que se envie todo el mensaje

#esta funcion le falta mejorar la definicion ya que es pseudocodigo de la cartilla
def read_line(skt):
    pos = buff.pos(",")
    if pos == 0:
        pos = buff.pos("}")
    if pos>0:
        line = buff.substr(1,pos-1)
        buff = buff.substr(pos+1,buff.len())
        return line, ""

    fin = time.now() + tiempo

    while (pos <= 0):
        skt.settimeout(fin,time.now())
        s, err = skt.listen()
        if err == "closed":
            return s, "closed"
        elif err == "timeout":
            return s, "timeout"
        buff = buff + s
        pos = buff.pos(",")

    line = buff.substr(1,pos-1)
    buff = buff.substr(pos+1,buff.len())
    return line

def send_all(skt,mensaje_a_enviar):
    mensaje_a_enviar = mensaje_a_enviar.encode()
    while mensaje_a_enviar.len() > 0:
        try:
            cant_bytes_enviados = skt.send(mensaje_a_enviar)
            mensaje_a_enviar = mensaje_a_enviar.substr(cant_bytes_enviados+1,mensaje_a_enviar.len())
        except socket.error:
            skt.close()
            return
        
funciones_implementadas = ["sum","echo","echo_concat"] #agregar 2 funciones con 2 parametros

def sum(*args):
    total = 0
    for number in args:
        total += number
    return total

def echo(mensaje):
    return mensaje

def echo_concat(*args):
    total = ""
    for mensaje in args:
        total += mensaje
    return total

#para recibir una solicitud como retornar parametros,
#nos tenemos que poner deacuerdo en el formato de
#envio y recepción de los Json
def retornar_parametros(*args):
    parametros = []
    while (args.substr("\n")>0): #asumo que se retiran parametros cuando aún queda el \n
        l, err = read_line(skt) #recibe el string del Json
        while (args.substr(",")>0): #separa el string de args en una lista de parametros
            parametros.insert(args)

    return parametros

def recibir_solicitud(skt):
    while (True):
        l, err = read_line(skt) #recibe el string del Json
        if err == "closed":
            return l, "closed"
        for funciones in funciones_implementadas:
            if l.substr(funciones)>0:
                nombre_funcion = l.sub(1,funciones.len())
                resto_l = l.sub(funciones.len()+2, l.len())
                funcion = locals().get(nombre_funcion)
                if callable(funcion):
                    parametros = retornar_parametros(resto_l)
                    funcion(*parametros) #realiza la funcion
                    return
                else:
                    return # deberia de retornar error pero luego vemos como hacemos

        
    



