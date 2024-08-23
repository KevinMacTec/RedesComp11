#funciones de cliente y servidor
import socket
import time

skt = socket
buff = ""
t = 3 #tiempo maximo para que se envie todo el mensaje

#skt envia un string de la forma "abc\ndef\n"
#server recibe "a" "ab" "abc" ""
def read_line(skt):
    pos = buff.pos("\n")
    if pos>0:
        line = buff.substr(1,pos-1)
        buff = buff.substr(pos+1,buff.len())
        return line

    fin = time.now() + t

    while (pos <= 0):
        skt.settimeout(fin,time.now())
        s, err = skt.listen()
        if err == "closed":
            return None, "closed"
        elif err == "timeout":
            return s, "timeout"
        buff = buff + s
        pos = buff.pos("\n")

    line = buff.substr(1,pos-1)
    buff = buff.substr(pos+1,buff.len())
    return line

def send_all(skt,s):
    remain = s
    while (remain!=""):
        remain, err = skt.send(remain)
        if err == "closed":
            return remain, "closed"
    return ""
    
    



