from jsonrpc_redes import connect

def test_client():
    # Este es el cliente de prueba que se ejecuta contra el
    # servidor de prueba en el módulo server.
    print('Aplicacion cliente')
    print('=============================')
    print('Iniciando pruebas de casos sin errores.')

    
    connS1 = connect('localhost', 8080)
    connS2 = connect('localhost', 8081)

    result = connS1.substract(20,10,3,2,1)
    assert result == (20-10-3-2-1)
    print('Test substract completado.')
    
    result = connS1.divide(20,2)
    assert result == 10
    print('Test de divide completado.')

    result = connS1.pow(base=2,exp=4)
    assert result == pow(2,4)
    print('Test de power_of completado.')

    result = connS1.nrt(16,2)
    assert result == pow(16,1/2)
    print('Test de n_root completado.')

    result = connS1.nrt(16,2, notify=True)
    assert result == None
    print('Test de notificación completado (servidor 1).')

    result = connS2.concat_str("hola"," mundo!")
    assert result == "hola mundo!"
    print('Test de concat simple completado.')

    result = connS2.concat_str("hola","mi","buen","amigo")
    assert result == "holamibuenamigo"
    print('Test de concat varios parametros completado.')

    result = connS2.repeat_string("hola",3)
    assert result == "holaholahola"
    print('Test de repeat completado.')

    result = connS2.truncate_string(string="hola mundo", pos=6)
    assert result == "hola"
    print('Test de truncate completado.')

    result = connS2.repeat_string("hola",3, notify=True)
    assert result == None
    print('Test de notificacion completado (servidor 2).')

    print('=============================')
    print('Pruebas de casos sin errores completadas.')
    print('=============================')
    print('Iniciando pruebas de casos con errores.')

    
    try:
        connS1.substract()
    except Exception as e:
        print('Llamada incorrecta sin parámetros. Genera excepción necesaria.')
        print(e.code, e.message)
    else:
        print('ERROR: No lanzó excepción.')
        
    try:
        connS2.substract(5, 6)
    except Exception as e:
        print('Llamada a método inexistente en servidor 2. Genera excepción necesaria.')
        print(e.code, e.message)
    else:
        print('ERROR: No lanzó excepción.')

    try:
        connS2.repeat_string('a', 'b', 'c')
    except Exception as e:
        print('Llamada incorrecta genera excepción interna del servidor.')
        print(e.code, e.message)
    else:
        print('ERROR: No lanzó excepción.')

    try:
        connS2.truncate_string('hola', pos=2)
    except Exception as e:
        print('Llamada incorrecta genera excepción en el cliente.')
        print(e)
    else:
        print('ERROR: No lanzó excepción.')

    try:
        connS1.pow(notify=True)
    except Exception as e:
        print('ERROR: Lanzó excepción.')
        print(e)
    else:
        print('Llamada incorrecta pero notifcacion, no genera error.')
    
    print('=============================')
    print("Pruebas de casos con errores completadas.")
    
if __name__ == "__main__":
    test_client()