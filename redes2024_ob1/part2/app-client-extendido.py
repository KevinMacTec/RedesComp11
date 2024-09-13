from jsonrpc_redes import connect
from math import pi

def test_client():
    # Este es el cliente de prueba que se ejecuta contra el
    # servidor de prueba en el módulo server.
    print('Aplicacion cliente')
    print('=============================')
    print('Iniciando pruebas de casos sin errores.')


    connS3 = connect('200.100.0.15', 8080)

    result = connS3.get_pi()
    assert result == (pi)
    print('Test get_pi completado. Sin parametros')
    
    result = connS3.circle_area_and_perimeter(1)
    assert result[0] == pi
    assert result[1] == 2*pi
    print('Test de circle_area_and_perimeter completado. Retorna mas de un valor')

    result = connS3.path_length(p1=(0,1), p2=(0,2), p3=(0,3))
    assert result == 2
    print('Test de path_length completado. Parametros con nombres no obligatorios')

    result = connS3.path_length()
    assert result == 0
    print('Test de parametros con nombres no obligatorios vacios.')

    result = connS3.get_pi(notify=True)
    assert result == None
    print('Test de notificación sin parametros completado.')

    result = connS3.path_length(p1=(1,1), p2=(2,2), p3=(4,4), p4=(8,8), notify = True)
    assert result == None
    print('Test de notificación con parametros completado.')

    print('=============================')
    print('Pruebas de casos sin errores completadas.')
    print('=============================')
    print('Iniciando pruebas de casos con errores.')

    
    try:
        connS3.substract(5, 6)
    except Exception as e:
        print('Llamada a método inexistente en servidor. Genera excepción necesaria.')
        print(e.code, e.message)
    else:
        print('ERROR: No lanzó excepción.')

    try:
        connS3.circle_area_and_perimeter(1,2)
    except Exception as e:
        print('Llamada con mas parametros de los requeridos genera excepción interna del servidor.')
        print(e.code, e.message)
    else:
        print('ERROR: No lanzó excepción.')

    try:
        connS3.circle_area_and_parameter(notify=True)
    except Exception as e:
        print('ERROR: Lanzó excepción.')
        print(e)
    else:
        print('Llamada incorrecta pero notifcacion, no genera error.')
    
    print('=============================')
    print("Pruebas de casos con errores completadas.")
    
if __name__ == "__main__":
    test_client()