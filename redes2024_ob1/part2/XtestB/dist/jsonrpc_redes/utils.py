def valid_count(mensaje,caracter):
    resultado = 0
    cant_posible_entre_comillas = 0
    contador = 0
    tipo_comilla = 0 # 0 == sin ; 1 == '' ; 2 == ""
    while(contador < len(mensaje)):
        if(tipo_comilla == 0):
            if(mensaje[contador] == "'"):
                tipo_comilla = 1
                cant_posible_entre_comillas = 0
            elif(mensaje[contador] == '"'):
                tipo_comilla = 2
                cant_posible_entre_comillas = 0
            elif(mensaje[contador] == caracter):
                resultado += 1
        elif (tipo_comilla == 1):
            if(mensaje[contador] == "'"):
                cant_posible_entre_comillas = 0
                tipo_comilla = 0
            elif(mensaje[contador] == caracter):
                cant_posible_entre_comillas += 1
        else:
            if(mensaje[contador] == '"'):
                cant_posible_entre_comillas = 0
                tipo_comilla = 0
            elif(mensaje[contador] == caracter):
                cant_posible_entre_comillas += 1
        contador += 1
    resultado += cant_posible_entre_comillas
    return resultado