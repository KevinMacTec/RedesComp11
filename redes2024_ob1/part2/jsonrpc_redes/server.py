#servidor
from jsonrpc_redes import client_server
import socket

funciones_implementadas = ["sum","echo","echo_concat"]

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
                    parametros = retornar_parametros()
                    funcion(*parametros) #realiza la funcion
                    return
                else:
                    return # deberia de retornar error pero luego vemos como hacemos


        