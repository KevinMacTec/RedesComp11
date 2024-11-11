/*-----------------------------------------------------------------------------
 * file: sr_pwospf.c
 *
 * Descripción:
 * Este archivo contiene las funciones necesarias para el manejo de los paquetes
 * OSPF.
 *
 *---------------------------------------------------------------------------*/

#include "sr_pwospf.h"
#include "sr_router.h"

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <malloc.h>

#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "sr_utils.h"
#include "sr_protocol.h"
#include "pwospf_protocol.h"
#include "sr_rt.h"
#include "pwospf_neighbors.h"
#include "pwospf_topology.h"
#include "dijkstra.h"

/* Variables de pwospf para el router son tratadas como
variables globales. Pueden ser accedidas desde todos los hilos */

/*pthread_t hello_thread;*/
pthread_t g_hello_packet_thread;
pthread_t g_all_lsu_thread;
pthread_t g_lsu_thread;
pthread_t g_neighbors_thread;
pthread_t g_topology_entries_thread;
pthread_t g_rx_lsu_thread;
pthread_t g_dijkstra_thread;

pthread_mutex_t g_dijkstra_mutex = PTHREAD_MUTEX_INITIALIZER;

struct in_addr g_router_id;
uint8_t g_ospf_multicast_mac[ETHER_ADDR_LEN];
struct ospfv2_neighbor* g_neighbors;
struct pwospf_topology_entry* g_topology;
uint16_t g_sequence_num;

/* -- Declaración de hilo principal de la función del subsistema pwospf. Si no
    lo agrego no la puedo llamar en init --- */
static void* pwospf_run_thread(void* arg);

/*---------------------------------------------------------------------
 * Method: pwospf_init(..)
 *
 * Configura las estructuras de datos internas para el subsistema pwospf
 * y crea un nuevo hilo para el subsistema pwospf.
 *
 * Se puede asumir que las interfaces han sido creadas e inicializadas
 * en este punto.
 *---------------------------------------------------------------------*/

int pwospf_init(struct sr_instance* sr)
{
    assert(sr);

    /* Reserva la memoria para el subsistema pwospf */
    /* pwospf_subsys contiene:
        Hilo del mutex del subsistema:
        Permite coordinar los accesos a los datos del subsistema (seran las
        variables globales)
        Hilo principal:
        Hilo principal del subsistema (el que inicializa todo)
     */
    sr->ospf_subsys = (struct pwospf_subsys*)malloc(sizeof(struct pwospf_subsys));

    /* Inicializa el hilo de mutex */
    assert(sr->ospf_subsys);
    pthread_mutex_init(&(sr->ospf_subsys->lock), 0);

    /* Inicializa el id del router como 0 (Luego sera modificado) */
    g_router_id.s_addr = 0;

    /* Define la MAC de multicast a usar para los paquetes HELLO */
    g_ospf_multicast_mac[0] = 0x01;
    g_ospf_multicast_mac[1] = 0x00;
    g_ospf_multicast_mac[2] = 0x5e;
    g_ospf_multicast_mac[3] = 0x00;
    g_ospf_multicast_mac[4] = 0x00;
    g_ospf_multicast_mac[5] = 0x05;

    /* Define la lista de vecinos como null */
    g_neighbors = NULL;

    /* Define el numero de secuencia como 0. El numero de secuencia se
    usa para marcar cada mensaje LSU generado por el router (no los que reenvia) */
    g_sequence_num = 0;


    /* Agrega un primer vecino zero */
    /* Neighbor contiene el id del router vecino, el tiempo desde que esta vivo y un
    puntero al siguiente neighbor */
    struct in_addr zero;
    zero.s_addr = 0;
    g_neighbors = create_ospfv2_neighbor(zero);
    /* Agrega zero a la representacion de la topologia que guarda el router */
    /* Topology guarda el registro de todos los routers en la topologia de red,
    en una lista de topology entries */
    g_topology = create_ospfv2_topology_entry(zero, zero, zero, zero, zero, 0);

    /* Inicializa el hilo principal */
    if( pthread_create(&sr->ospf_subsys->thread, 0, pwospf_run_thread, sr)) { 
        perror("pthread_create");
        assert(0);
    }

    /* Exito */
    return 0; 
} /* -- pwospf_init -- */


/*---------------------------------------------------------------------
 * Method: pwospf_lock
 *
 * Lock mutex associated with pwospf_subsys
 *
 *---------------------------------------------------------------------*/

void pwospf_lock(struct pwospf_subsys* subsys)
{
    if ( pthread_mutex_lock(&subsys->lock) )
    { assert(0); }
}

/*---------------------------------------------------------------------
 * Method: pwospf_unlock
 *
 * Unlock mutex associated with pwospf subsystem
 *
 *---------------------------------------------------------------------*/

void pwospf_unlock(struct pwospf_subsys* subsys)
{
    if ( pthread_mutex_unlock(&subsys->lock) )
    { assert(0); }
} 

/*---------------------------------------------------------------------
 * Method: pwospf_run_thread
 *
 * Hilo principal del subsistema pwospf.
 * Inicializa los procesos del subsistema pwospf.
 * 
 *---------------------------------------------------------------------*/

static
void* pwospf_run_thread(void* arg)
{
    sleep(5);

    struct sr_instance* sr = (struct sr_instance*)arg;

    /* Set the ID of the router */
    while(g_router_id.s_addr == 0)
    {
        struct sr_if* int_temp = sr->if_list;
        while(int_temp != NULL)
        {
            if (int_temp->ip > g_router_id.s_addr)
            {
                g_router_id.s_addr = int_temp->ip;
            }

            int_temp = int_temp->next;
        }
    }
    Debug("\n\nPWOSPF: Selecting the highest IP address on a router as the router ID\n");
    Debug("-> PWOSPF: The router ID is [%s]\n", inet_ntoa(g_router_id));


    Debug("\nPWOSPF: Detecting the router interfaces and adding their networks to the routing table\n");
    struct sr_if* int_temp = sr->if_list;
    while(int_temp != NULL)
    {
        struct in_addr ip;
        ip.s_addr = int_temp->ip;
        struct in_addr gw;
        gw.s_addr = 0x00000000;
        struct in_addr mask;
        mask.s_addr =  int_temp->mask;
        struct in_addr network;
        network.s_addr = ip.s_addr & mask.s_addr;

        if (check_route(sr, network) == 0)
        {
            Debug("-> PWOSPF: Adding the directly connected network [%s, ", inet_ntoa(network));
            Debug("%s] to the routing table\n", inet_ntoa(mask));
            sr_add_rt_entry(sr, network, gw, mask, int_temp->name, 1);
        }
        int_temp = int_temp->next;
    }
    
    Debug("\n-> PWOSPF: Printing the forwarding table\n");
    sr_print_routing_table(sr);


    pthread_create(&g_hello_packet_thread, NULL, send_hellos, sr);
    pthread_create(&g_all_lsu_thread, NULL, send_all_lsu, sr);
    pthread_create(&g_neighbors_thread, NULL, check_neighbors_life, sr);
    pthread_create(&g_topology_entries_thread, NULL, check_topology_entries_age, sr);

    return NULL;
} /* -- run_ospf_thread -- */

/***********************************************************************************
 * Métodos para el manejo de los paquetes HELLO y LSU
 * SU CÓDIGO DEBERÍA IR AQUÍ
 * *********************************************************************************/

/*---------------------------------------------------------------------
 * Method: print_neighbors
 *
 * Imprime la lista de vecinos del router
 *
 *---------------------------------------------------------------------*/
void print_neighbors(struct sr_instance* sr)
{
    Debug("Neighbors list for router %s\n", inet_ntoa(g_router_id));
    struct ospfv2_neighbor * ngbr = g_neighbors;
    int i = 0;
    while (ngbr != NULL) {
        Debug("      [Neighbor ID = %s]\n", inet_ntoa(ngbr->neighbor_id));
        Debug("         [State = %d]\n", ngbr->alive);    
        struct sr_if* iface = sr->if_list;
            while (iface != NULL) {
                if (iface->neighbor_id == ngbr->neighbor_id.s_addr){
                    Debug("         [On interface = %s]\n", iface->name);    
                }         
                /* Paso a la siguiente interfaz */
                iface = iface->next;       
            }
        i ++;
        ngbr = ngbr->next;
    }
    Debug("Neighbor count = %d]\n", i);
}

/*---------------------------------------------------------------------
 * Method: check_neighbors_life
 *
 * Chequea si los vecinos están vivos
 *
 *---------------------------------------------------------------------*/

void* check_neighbors_life(void* arg)
{
    struct sr_instance* sr = (struct sr_instance*)arg;

    /* Loop constante */
    while (1){
        /* Cada 1 segundo */
        usleep(1000000);

        Debug("Checking neighbors lives...\n");
        /* Chequeo lista de vecinos */
        /* Check Neighbors Alive recorre la lista de vecinos y elimina aquellos
        vecinos que tienen tiempo de vida (restante) igual a 0. Si el tiempo de
        vida no es igual a cero entonces lo decrementa en uno. */
        /* Ahora retorna vecinos eliminados */
        /* Tengan en cuenta que la lista es una copia y deben gestionar ustedes la memoria. */
        struct ospfv2_neighbor* deleted_ngbrs = check_neighbors_alive(g_neighbors);

        /* Si hay un cambio, se debe ajustar el neighbor id en la interfaz. */
        while (deleted_ngbrs != NULL) {
            /* Tomo el id del vecino */
            uint32_t deleted_id = deleted_ngbrs->neighbor_id.s_addr;
            /* Busco que interfaces tienen ese vecino y las actualizo */
            struct sr_if* iface = sr->if_list;
            while (iface != NULL) {
                if (iface->neighbor_id == deleted_id){
                    /* Seteo en 0 IP e Id */
                    iface->neighbor_id = 0;
                    iface->neighbor_ip = 0;

                    /* Veo los vecinos */
                    print_neighbors(sr);
                }         
                /* Paso a la siguiente interfaz */
                iface = iface->next;       
            }
            /* Paso al siguiente vecino eliminado */
            deleted_ngbrs = deleted_ngbrs->next;
        }

        /* Libero la memoria de la lista de vecinos eliminados */
        free(deleted_ngbrs);
    }
    return NULL;
} /* -- check_neighbors_life -- */


/*---------------------------------------------------------------------
 * Method: check_topology_entries_age
 *
 * Check if the topology entries are alive 
 * and if they are not, remove them from the topology table
 *
 *---------------------------------------------------------------------*/

void* check_topology_entries_age(void* arg)
{
    struct sr_instance* sr = (struct sr_instance*)arg;
    /* Loop constante */
    while (1){
        /* Cada 1 segundo */
        usleep(1000000);

        Debug("Checking topology entries ages...\n");
        /* Chequea el tiempo de vida de cada entrada de la topologia. */
        /* Check Topology Age recorre la lista de topology entries y elimina 
        aquellas entradas que tienen tiempo de vida igual al tiempo maximo. Si 
        el tiempo de vida no es igual al maximo entonces lo aumenta en uno.
        Retorna 1 si hubo alguna eliminacion */
        u_int8_t change = check_topology_age(g_topology);
        /* Si hay un cambio en la topología, se llama a la función de Dijkstra
        en un nuevo hilo. */
        if (change) {
            dijkstra_param_t* dij = malloc(sizeof(dijkstra_param_t));
            /* Almaceno las variables en la estructura para dijkstra */
            dij->sr = sr;
            dij->topology = g_topology;
            dij->mutex = g_dijkstra_mutex;
            /* El id del router se guarda en un struct */
            struct in_addr rid;
            rid.s_addr = sr->topo_id;
            dij->rid = rid;
            /* Llamo a la funcion en un nuevo hilo */
            /* pthread_create crea un hilo nuevo para ejecutar la funcion que
            se pasa como parametro (3er param). En el primer parametro se guarda
            un puntero al identificador del proceso. El segundo parametro son arugumentos
            que se pasan para crear el hilo (NULL pasa args por default). El ultimo parametro
            son los parametros que recibe la funcion que se ejecuta en el hilo. */
            pthread_create(&g_dijkstra_thread, NULL, run_dijkstra, dij);
            
            /* Se imprime la topología resultado del chequeo */
            Debug("Printing the resulting topology table: \n");
            print_topolgy_table(g_topology);
            Debug("\n");
        }
    }
    return NULL;
} /* -- check_topology_entries_age -- */


/*---------------------------------------------------------------------
 * Method: send_hellos
 *
 * Para cada interfaz y cada helloint segundos, construye mensaje 
 * HELLO y crea un hilo con la función para enviar el mensaje.
 *
 *---------------------------------------------------------------------*/

void* send_hellos(void* arg)
{
    struct sr_instance* sr = (struct sr_instance*)arg;

    /* Loop constante */
    while(1) {
        /* Cada 1 segundo */
        usleep(1000000);

        /* Bloqueo para evitar mezclar el envío de HELLOs y LSUs */
        pwospf_lock(sr->ospf_subsys);

        /* Para todas las interfaces */
        struct sr_if* iface = sr->if_list;
        while (iface != NULL) {
            /* Envio el paquete HELLO */
            powspf_hello_lsu_param_t* hello_param = malloc(sizeof(powspf_hello_lsu_param_t));
            hello_param->sr = sr;
            hello_param->interface = iface;
            /* En un hilo independiente. Donde guardo el puntero al hilo? */
            pthread_create(&g_hello_packet_thread, NULL, send_hello_packet, hello_param);
            /* Cada interfaz matiene un contador en segundos para los HELLO*/
            /* Reiniciar el contador de segundos para HELLO */
            iface->helloint = OSPF_DEFAULT_HELLOINT; /* ? */

            /* Paso a la siguiente interfaz */
            iface = iface->next;
        }

        /* Desbloqueo */
        pwospf_unlock(sr->ospf_subsys);
    };

    return NULL;
} /* -- send_hellos -- */


/*---------------------------------------------------------------------
 * Method: send_hello_packet
 *
 * Recibe un mensaje HELLO, agrega cabezales y lo envía por la interfaz
 * correspondiente.
 *
 *---------------------------------------------------------------------*/

void* send_hello_packet(void* arg)
{
    powspf_hello_lsu_param_t* hello_param = ((powspf_hello_lsu_param_t*)(arg));
    struct sr_if* iface = hello_param->interface;
    Debug("\n\nPWOSPF: Constructing HELLO packet for interface %s\n", iface->name);

    /* Tamaño del paquete HELLO */ 
    unsigned int packet_len = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(ospfv2_hdr_t) + sizeof(ospfv2_hello_hdr_t);
    uint8_t* packet = malloc(packet_len);
    /* Encabezado Ethernet */
    sr_ethernet_hdr_t* eth_hdr = (sr_ethernet_hdr_t*) packet;
    /* Seteo la dirección MAC de multicast para la trama a enviar */
    memcpy(eth_hdr->ether_dhost, g_ospf_multicast_mac, ETHER_ADDR_LEN);
    /* Seteo la dirección MAC origen con la dirección de mi interfaz de salida */
    memcpy(eth_hdr->ether_shost, iface->addr, ETHER_ADDR_LEN);
    /* Seteo el ether_type en el cabezal Ethernet */
    eth_hdr->ether_type = htons(ethertype_ip);                           

    /* Encabezado IP */
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t*) (packet + sizeof(sr_ethernet_hdr_t));   
    /* Inicializo cabezal IP */
    ip_hdr->ip_v = 4;                                   
    ip_hdr->ip_hl = sizeof(sr_ip_hdr_t)/4;            
    ip_hdr->ip_tos = 0;                                 
    ip_hdr->ip_len = htons(packet_len - sizeof(sr_ethernet_hdr_t));
    ip_hdr->ip_id = htons(0); /* EVALUAR MANTENER UN CONTADOR DE ID IP GLOBAL */                           
    ip_hdr->ip_off = htons(0);                                 
    ip_hdr->ip_ttl = 16;
    /* Seteo IP origen con la IP de mi interfaz de salida */
     ip_hdr->ip_src = iface->ip;                         
    /* Seteo IP destino con la IP de Multicast dada: OSPF_AllSPFRouters  */
    ip_hdr->ip_dst = htonl(OSPF_AllSPFRouters); 
    /* Seteo el protocolo en el cabezal IP para ser el de OSPF (89) */
    ip_hdr->ip_p = ip_protocol_ospfv2;    
    /* Calculo y seteo el chechsum IP*/            
    ip_hdr->ip_sum = ip_cksum(ip_hdr, 4 * ip_hdr->ip_hl);

    /* Encabezado PWOSPF */
    ospfv2_hdr_t* ospf_hdr = (ospfv2_hdr_t*) (packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
    /* Inicializo cabezal de PWOSPF con version 2 y tipo HELLO */
    ospf_hdr->version = OSPF_V2;
    ospf_hdr->type = OSPF_TYPE_HELLO;                   
    ospf_hdr->len = htons(sizeof(ospfv2_hdr_t) + sizeof(ospfv2_hello_hdr_t));
    /* Seteo el Router ID con mi ID*/
    ospf_hdr->rid = g_router_id.s_addr;   /* HTONS? */            
    /* Seteo el Area ID en 0 */
    ospf_hdr->aid = 0;                           
    /* Seteo el Authentication Type y Authentication Data en 0*/
    ospf_hdr->autype = 0;
    ospf_hdr->audata = 0;

    /* Encabezado HELLO */
    ospfv2_hello_hdr_t* hello_hdr = (ospfv2_hello_hdr_t*) (packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(ospfv2_hdr_t));
    /* Seteo máscara con la máscara de mi interfaz de salida */
    hello_hdr->nmask = iface->mask;
    /* Seteo Hello Interval con OSPF_DEFAULT_HELLOINT */
    hello_hdr->helloint = htons(OSPF_DEFAULT_HELLOINT);
    /* Seteo Padding en 0 */
    hello_hdr->padding = 0;

    /* Creo el paquete a transmitir */
    /* Calculo y actualizo el checksum del cabezal OSPF */
    ospf_hdr->csum = ospfv2_cksum(ospf_hdr, sizeof(ospfv2_hdr_t) + sizeof(ospfv2_hello_hdr_t));

    /* Envío el paquete HELLO */
    sr_send_packet(hello_param->sr, packet, packet_len, iface->name);

    /* Imprimo información del paquete HELLO enviado */
 /*    Debug("-> PWOSPF: Sending HELLO Packet of length = %d, out of the interface: %s\n", packet_len, hello_param->interface->name);
    Debug("      [Router ID = %s]\n", inet_ntoa(g_router_id));
    struct in_addr router_ip;
    router_ip.s_addr = ip_hdr->ip_src;
    Debug("      [Router IP = %s]\n", inet_ntoa(router_ip));
    struct in_addr net_mask;
    net_mask.s_addr = hello_hdr->nmask;
    Debug("      [Network Mask = %s]\n", inet_ntoa(net_mask));

    Debug("-> PWOSPF: HELLO Packet sent on interface: %s\n", iface->name);
  */   free(packet);
    /* Antes de llamar al hilo de esta funcion cree memoria dinamica para el parametro */
    free(hello_param);

    return NULL;

} /* -- send_hello_packet -- */

/*---------------------------------------------------------------------
 * Method: send_all_lsu
 *
 * Construye y envía LSUs cada 30 segundos
 *
 *---------------------------------------------------------------------*/

void* send_all_lsu(void* arg)
{
    struct sr_instance* sr = (struct sr_instance*)arg;

    /* while true*/
    while(1) {
        /* Se ejecuta cada OSPF_DEFAULT_LSUINT segundos */
        usleep(OSPF_DEFAULT_LSUINT * 1000000);

        /* Bloqueo para evitar mezclar el envío de HELLOs y LSUs */
        pwospf_lock(sr->ospf_subsys);
        
        /* Para todas las interfaces para enviar el paquete LSU */
            /* Si la interfaz tiene un vecino, envío un LSU */
            struct sr_if* iface = sr->if_list;
            while (iface) {
                /* Avanzo en la lista de vecinos activos*/
                struct ospfv2_neighbor* neighbor = g_neighbors;
                while (neighbor) {
                    /* Enviar LSU si hay un vecino activo */
                    if (neighbor->alive) {
                        powspf_hello_lsu_param_t* lsu_param = malloc(sizeof(powspf_hello_lsu_param_t));
                        lsu_param->sr = sr;
                        lsu_param->interface = iface;
                        /* Crea un hilo de ejecución para enviar el LSU */
                        pthread_t lsu_thread;
                        pthread_create(&lsu_thread, NULL, send_lsu, lsu_param);
                        pthread_detach(lsu_thread); /* Evita fuga de memoria */
                        break;
                    }                
                    neighbor = neighbor->next;
                }
                iface = iface->next;
            }

        /* Desbloqueo para poder seguir enviando HELLOs*/
        pwospf_unlock(sr->ospf_subsys);
    }

    return NULL;
} /* -- send_all_lsu -- */

/*---------------------------------------------------------------------
 * Method: send_lsu
 *
 * Construye y envía paquetes LSU a través de una interfaz específica
 *
 *---------------------------------------------------------------------*/

void* send_lsu(void* arg)
{
    powspf_hello_lsu_param_t* lsu_param = ((powspf_hello_lsu_param_t*)(arg));

    /* Solo envío LSUs si del otro lado hay un router*/
    
    /* Construyo el LSU */
    Debug("\n\nPWOSPF: Constructing LSU packet\n");

    /* Inicializo cabezal Ethernet */
    /* Dirección MAC destino la dejo para el final ya que hay que hacer ARP */

    /* Inicializo cabezal IP*/
    /* La IP destino es la del vecino contectado a mi interfaz*/
   
    /* Inicializo cabezal de OSPF*/

    /* Seteo el número de secuencia y avanzo*/
    /* Seteo el TTL en 64 y el resto de los campos del cabezal de LSU */
    /* Seteo el número de anuncios con la cantidad de rutas a enviar. Uso función count_routes */

    /* Creo el paquete y seteo todos los cabezales del paquete a transmitir */

    /* Creo cada LSA iterando en las enttadas de la tabla */
        /* Solo envío entradas directamente conectadas y agreagadas a mano*/
        /* Creo LSA con subnet, mask y routerID (id del vecino de la interfaz)*/

    /* Calculo el checksum del paquete LSU */

    /* Me falta la MAC para poder enviar el paquete, la busco en la cache ARP*/
    /* Envío el paquete si obtuve la MAC o lo guardo en la cola para cuando tenga la MAC*/
   
   /* Libero memoria */

    return NULL;
} /* -- send_lsu -- */


/*---------------------------------------------------------------------
 * Method: sr_handle_pwospf_hello_packet
 *
 * Gestiona los paquetes HELLO recibidos
 *
 *---------------------------------------------------------------------*/

void sr_handle_pwospf_hello_packet(struct sr_instance* sr, uint8_t* packet, unsigned int length, struct sr_if* rx_if)
{
    /* Obtengo información del paquete recibido */
    /* Tomo el cabezal IP */
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t *)(sizeof(sr_ethernet_hdr_t) + packet);
    /* Tomo el cabezal PWOSPF */
    ospfv2_hdr_t* pwospf_hdr = (ospfv2_hdr_t*) (packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
    /* Y el cabezal HELLO */
    ospfv2_hello_hdr_t* hello_hdr = (ospfv2_hello_hdr_t*) (packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(ospfv2_hdr_t));

    struct in_addr neighbor_id;
    neighbor_id.s_addr = pwospf_hdr->rid;
    struct in_addr neighbor_ip;
    neighbor_ip.s_addr = ip_hdr->ip_src;
    struct in_addr net_mask;
    net_mask.s_addr = hello_hdr->nmask;
    /* Imprimo info del paquete recibido*/
 /*    Debug("-> PWOSPF: Detecting PWOSPF HELLO Packet from:\n");
    Debug("      [Neighbor ID = %s]\n", inet_ntoa(neighbor_id));
    Debug("      [Neighbor IP = %s]\n", inet_ntoa(neighbor_ip));
    Debug("      [Network Mask = %s]\n", inet_ntoa(net_mask));
 */
    /* Chequeo checksum */
    if (ospfv2_cksum(pwospf_hdr, sizeof(ospfv2_hdr_t)) == pwospf_hdr->csum) {
        Debug("-> PWOSPF: HELLO Packet dropped, invalid checksum\n");
        return;
    }
    /* Chequeo de la máscara de red */
    /* NTOHL? NO*/
    if (rx_if->mask != net_mask.s_addr) {
        Debug("-> PWOSPF: HELLO Packet dropped, invalid hello network mask\n");
        Debug("Expected: %d, Received: %d\n", rx_if->mask, net_mask.s_addr);
        return;
    }
    /* Chequeo del intervalo de HELLO */
    /* NTOHL? SI*/
    if (rx_if->helloint != ntohs(hello_hdr->helloint)) {
        Debug("-> PWOSPF: HELLO Packet dropped, invalid hello interval\n");
        Debug("Expected: %d, Received: %d\n", rx_if->helloint, hello_hdr->helloint);
        return;
    }
    Debug("-> PWOSPF: Valid HELLO Packet.\n");
    /* Si es un nuevo vecino */
    if (rx_if->neighbor_id != neighbor_id.s_addr || rx_if->neighbor_ip != neighbor_ip.s_addr) {
    /* Seteo el vecino en la interfaz por donde llegó */
    rx_if->neighbor_id = neighbor_id.s_addr;
    rx_if->neighbor_ip = neighbor_ip.s_addr;
    }
    /* Actualizo la lista de vecinos */
    /* Refresh neighbors alive recorre la lista de vecinos buscando el vecino que se 
    indica por id. Si lo encuentra actualiza el tiempo de vida, si no lo agrega a la
    lista de vecinos */
    refresh_neighbors_alive(g_neighbors, neighbor_id);
    /* Debo enviar LSUs por todas mis interfaces */
    struct sr_if* iface = sr->if_list;
    /* Recorro todas las interfaces para enviar el paquete LSU */
    while (iface != NULL) {
        /* Si la interfaz tiene un vecino, envío un LSU */
        if (iface->neighbor_id != 0) {
            powspf_hello_lsu_param_t* lsu_param = malloc(sizeof(powspf_hello_lsu_param_t));;
            lsu_param->interface = iface;
            lsu_param->sr = sr;
            pthread_create(&g_lsu_thread, NULL, send_lsu, lsu_param);
        }
        /* Paso a la siguiente interfaz */
        iface = iface->next;
    } 

   /* Veo los vecinos */
    print_neighbors(sr);
     
} /* -- sr_handle_pwospf_hello_packet -- */


/*---------------------------------------------------------------------
 * Method: sr_handle_pwospf_lsu_packet
 *
 * Gestiona los paquetes LSU recibidos y actualiza la tabla de topología
 * y ejecuta el algoritmo de Dijkstra
 *
 *---------------------------------------------------------------------*/

void* sr_handle_pwospf_lsu_packet(void* arg)
{
    powspf_rx_lsu_param_t* rx_lsu_param = ((powspf_rx_lsu_param_t*)(arg));

    /* Obtengo el vecino que me envió el LSU*/
    /* Imprimo info del paquete recibido*/
    /*
    Debug("-> PWOSPF: Detecting LSU Packet from [Neighbor ID = %s, IP = %s]\n", inet_ntoa(next_hop_id), inet_ntoa(next_hop_ip));
    */
    
    /* Chequeo checksum */
        /*Debug("-> PWOSPF: LSU Packet dropped, invalid checksum\n");*/

    /* Obtengo el Router ID del router originario del LSU y chequeo si no es mío*/
        /*Debug("-> PWOSPF: LSU Packet dropped, originated by this router\n");*/

    /* Obtengo el número de secuencia y uso check_sequence_number para ver si ya lo recibí desde ese vecino*/
        /*Debug("-> PWOSPF: LSU Packet dropped, repeated sequence number\n");*/

    /* Itero en los LSA que forman parte del LSU. Para cada uno, actualizo la topología.*/
    /*Debug("-> PWOSPF: Processing LSAs and updating topology table\n");*/        
        /* Obtengo subnet */
        /* Obtengo vecino */
        /* Imprimo info de la entrada de la topología */
        /*
        Debug("      [Subnet = %s]", inet_ntoa(net_num));
        Debug("      [Mask = %s]", inet_ntoa(net_mask));
        Debug("      [Neighbor ID = %s]\n", inet_ntoa(neighbor_id));
        */
        /* LLamo a refresh_topology_entry*/

    /* Imprimo la topología */
    /*
    Debug("\n-> PWOSPF: Printing the topology table\n");
    print_topolgy_table(g_topology);
    */


    /* Ejecuto Dijkstra en un nuevo hilo (run_dijkstra)*/

    /* Flooding del LSU por todas las interfaces menos por donde me llegó */
            /* Seteo MAC de origen */
            /* Ajusto paquete IP, origen y checksum*/
            /* Ajusto cabezal OSPF: checksum y TTL*/
            /* Envío el paquete*/
            
    return NULL;
} /* -- sr_handle_pwospf_lsu_packet -- */

/**********************************************************************************
 * SU CÓDIGO DEBERÍA TERMINAR AQUÍ
 * *********************************************************************************/

/*---------------------------------------------------------------------
 * Method: sr_handle_pwospf_packet
 *
 * Gestiona los paquetes PWOSPF
 *
 *---------------------------------------------------------------------*/

void sr_handle_pwospf_packet(struct sr_instance* sr, uint8_t* packet, unsigned int length, struct sr_if* rx_if)
{
    /*Nuevo. Si aún no terminó la inicialización, se descarta el paquete recibido */
    if (g_router_id.s_addr == 0) {
       return;
    }

    ospfv2_hdr_t* rx_ospfv2_hdr = ((ospfv2_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t)));
    powspf_rx_lsu_param_t* rx_lsu_param = ((powspf_rx_lsu_param_t*)(malloc(sizeof(powspf_rx_lsu_param_t))));

    Debug("-> PWOSPF: Detecting PWOSPF Packet\n");
    Debug("      [Type = %d]\n", rx_ospfv2_hdr->type);

    switch(rx_ospfv2_hdr->type)
    {
        case OSPF_TYPE_HELLO:
            sr_handle_pwospf_hello_packet(sr, packet, length, rx_if);
            break;
        case OSPF_TYPE_LSU:
            rx_lsu_param->sr = sr;
            unsigned int i;
            for (i = 0; i < length; i++)
            {
                rx_lsu_param->packet[i] = packet[i];
            }
            rx_lsu_param->length = length;
            rx_lsu_param->rx_if = rx_if;
            /* Nuevo */
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_t pid;
            pthread_create(&pid, &attr, sr_handle_pwospf_lsu_packet, rx_lsu_param);

            break;
    }
} /* -- sr_handle_pwospf_packet -- */
