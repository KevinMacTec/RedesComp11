/**********************************************************************
 * file:  sr_router.c
 *
 * Descripción:
 *
 * Este archivo contiene todas las funciones que interactúan directamente
 * con la tabla de enrutamiento, así como el método de entrada principal
 * para el enrutamiento.
 *
 **********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

static uint16_t ip_id_counter = 0;

uint8_t *generate_icmp_packet(uint8_t type,                  /* Nº de tipo de mensaje ICMP */
                            uint8_t code,                   /* Código dentro del tipo ICMP */
                            uint8_t* packet,                /* Puntero al paquete recibido */
                            struct sr_instance* sr,         /* Puntero a la instancia del router */
                            struct sr_if* interface){       /* Puntero a la interface  */

    /* Tomo el cabezal Ethernet del paquete recibido */
    sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t*) (packet);

    /* Seteo las direcciones MAC inversamente a como las recibi pues ahora voy a responder */
    uint8_t* src_MAC = eth_hdr->ether_dhost; 
    uint8_t* dest_MAC = eth_hdr->ether_shost;

    /* Seteo la direccion IP de origen como la asociada a la interfaz, y la de destinto como la de origen del paquete recibido */
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t *)(sizeof(sr_ethernet_hdr_t) + packet);
    uint32_t src_ip = interface->ip;
    uint32_t dest_ip = ip_hdr->ip_src;

    /* Marco el inicio del payload ICMP */
    uint8_t* icmp_data = packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
    /* Calculo el largo del payload ICMP */
    unsigned long icmp_data_len = ntohs(ip_hdr->ip_len) - ip_hdr->ip_hl*4 - sizeof(sr_icmp_hdr_t);

    /* Reservo memoria para el paquete que voy a crear */
    uint8_t* packet_icmp = malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t) + icmp_data_len);

    /* Creo el cabezal Ethernet */
    sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t*) (packet_icmp);
    memcpy(new_eth_hdr->ether_shost, src_MAC, sizeof(uint8_t) * ETHER_ADDR_LEN);
    memcpy(new_eth_hdr->ether_dhost, dest_MAC, sizeof(uint8_t) * ETHER_ADDR_LEN);
    new_eth_hdr->ether_type = htons(ethertype_ip);

    /* Creo el cabezal IP */
    sr_ip_hdr_t* new_ip_hdr = (sr_ip_hdr_t *)(packet_icmp + sizeof(sr_ethernet_hdr_t));
    /* memcpy(new_ip_hdr, ip_hdr, sizeof(sr_ip_hdr_t)); */
    new_ip_hdr->ip_tos = 0;
    new_ip_hdr->ip_ttl = 16;
    new_ip_hdr->ip_hl = 5;
    new_ip_hdr->ip_v = 4;
    new_ip_hdr->ip_len = htons(sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t) + icmp_data_len);
    new_ip_hdr->ip_p = ip_protocol_icmp;
    new_ip_hdr->ip_src = src_ip;
    new_ip_hdr->ip_dst = dest_ip;
    new_ip_hdr->ip_id = htons(ip_id_counter++);
    new_ip_hdr->ip_off = 0;
    /* Segun chatGPT cksum ya devuelve el resultado en network byte order */
    new_ip_hdr->ip_sum = ip_cksum(new_ip_hdr, 4 * new_ip_hdr->ip_hl);

    sr_icmp_hdr_t *icmp_hdr = (sr_icmp_hdr_t *)( packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
    /* Creo el cabezal ICMP */
    sr_icmp_hdr_t *new_icmp_hdr = (sr_icmp_hdr_t *)( packet_icmp + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
    new_icmp_hdr->icmp_code = code;
    new_icmp_hdr->icmp_type = type;
    /* No preciso htons porque los copio tal cual vinieron */
    new_icmp_hdr->icmp_id = icmp_hdr->icmp_id;
    new_icmp_hdr->icmp_seq = icmp_hdr->icmp_seq;
    /* Copio el payload */
    uint8_t* new_icmp_data = packet_icmp + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
    memcpy(new_icmp_data, icmp_data, icmp_data_len);
    new_icmp_hdr->icmp_sum = icmp_cksum(new_icmp_hdr, sizeof(sr_icmp_hdr_t) + icmp_data_len);

    return packet_icmp;
}

uint8_t *generate_icmp_t3_packet(uint8_t type,                  /* Nº de tipo de mensaje ICMP */
                                uint8_t code,                   /* Código dentro del tipo ICMP */
                                uint8_t* packet,                /* Puntero al paquete recibido */
                                struct sr_instance* sr,         /* Puntero a la instancia del router */
                                struct sr_if* interface){       /* Puntero a la interface  */

    /* Tomo el cabezal Ethernet del paquete recibido */
    sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t*) (packet);

    /* Seteo las direcciones MAC inversamente a como las recibi pues ahora voy a responder */
    uint8_t* src_MAC = eth_hdr->ether_dhost; 
    uint8_t* dest_MAC = eth_hdr->ether_shost;

    /* Seteo la direccion IP de origen como la asociada a la interfaz, y la de destinto como la de origen del paquete recibido */
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t *)(sizeof(sr_ethernet_hdr_t) + packet);
    uint32_t src_ip = interface->ip;
    uint32_t dest_ip = ip_hdr->ip_src;

    /* Reservo memoria para el paquete que voy a crear */
    uint8_t* packet_icmp = malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_t3_hdr_t));

    /* Creo el cabezal Ethernet */
    sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t*) (packet_icmp);
    memcpy(new_eth_hdr->ether_shost, src_MAC, sizeof(uint8_t) * ETHER_ADDR_LEN);
    memcpy(new_eth_hdr->ether_dhost, dest_MAC, sizeof(uint8_t) * ETHER_ADDR_LEN);
    new_eth_hdr->ether_type = htons(ethertype_ip);

    /* Creo el cabezal IP */
    sr_ip_hdr_t* new_ip_hdr = (sr_ip_hdr_t *)(packet_icmp + sizeof(sr_ethernet_hdr_t));
    /* memcpy(new_ip_hdr, ip_hdr, sizeof(sr_ip_hdr_t)); */
    new_ip_hdr->ip_tos = 0;
    new_ip_hdr->ip_ttl = 16;
    new_ip_hdr->ip_hl = 5;
    new_ip_hdr->ip_v = 4;
    new_ip_hdr->ip_len = htons(sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_t3_hdr_t));
    new_ip_hdr->ip_p = ip_protocol_icmp;
    new_ip_hdr->ip_src = src_ip;
    new_ip_hdr->ip_dst = dest_ip;
    new_ip_hdr->ip_id = htons(ip_id_counter++);
    new_ip_hdr->ip_off = 0;
    /* Segun chatGPT cksum ya devuelve el resultado en network byte order */
    new_ip_hdr->ip_sum = ip_cksum(new_ip_hdr, 4 * new_ip_hdr->ip_hl);

    /* Creo el cabezal ICMP */
    sr_icmp_t3_hdr_t *new_icmp_t3_hdr = (sr_icmp_t3_hdr_t *)( packet_icmp + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
    new_icmp_t3_hdr->icmp_code = code;
    new_icmp_t3_hdr->icmp_type = type;
    /* Si ICMP distinto de codigo 4 (Fragmentation Needed) no es necesario valor particular de next_mtu */
    if (code != 4){
        new_icmp_t3_hdr->next_mtu = 0;
    } else {
        /* A chequear */
        new_icmp_t3_hdr->next_mtu = 0;
    }
    new_icmp_t3_hdr->unused = 0;
    /* Para data copio los primeros 28 bytes partiendo de la cabecera IP (cabecera IP + 8 bytes de mensaje siguiente) */
    memcpy(new_icmp_t3_hdr->data, ip_hdr, ICMP_DATA_SIZE);
    new_icmp_t3_hdr->icmp_sum = icmp3_cksum(new_icmp_t3_hdr, sizeof(sr_icmp_t3_hdr_t));
    
    return packet_icmp;
}

void delete_icmp_packet(uint8_t* icmp_packet){
    uint8_t* icmp_data = icmp_packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
    free(icmp_packet);
}

void delete_icmp_t3_packet(uint8_t* icmp_t3_packet){
    free(icmp_t3_packet);
}

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Inicializa el subsistema de enrutamiento
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    assert(sr);

    /* Inicializa la caché y el hilo de limpieza de la caché */
    sr_arpcache_init(&(sr->cache));

    /* Inicializa los atributos del hilo */
    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    /* Hilo para gestionar el timeout del caché ARP */
    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);

} /* -- sr_init -- */

/* Envía un paquete ICMP de error */
void sr_send_icmp_error_packet(uint8_t type,
                              uint8_t code,
                              struct sr_instance *sr,
                              uint32_t ipDst,
                              uint8_t *ipPacket)
{
  uint8_t eth_hdr = ((sr_ethernet_hdr_t*)(ipPacket))->ether_dhost;
  struct sr_if* interface = sr_get_interface(sr, lpm(sr, ipDst)->interface);
  uint8_t* icmp_packet = generate_icmp_packet_t3(type, code, ipPacket, sr, interface);
  unsigned int icmp_len_t3 = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_t3_hdr_t);
  sr_send_packet(sr, icmp_packet, icmp_len_t3, interface->name);
  free(icmp_packet);
} /* -- sr_send_icmp_error_packet -- */

void sr_handle_ip_packet(struct sr_instance *sr,
        uint8_t *packet,              
        unsigned int len,
        uint8_t *srcAddr,
        uint8_t *destAddr,
        char *interface,
        sr_ethernet_hdr_t *eHdr)
{
  /* 
  * COLOQUE ASÍ SU CÓDIGO
  * SUGERENCIAS: 
  * - Obtener el cabezal IP y direcciones 
  * - Verificar si el paquete es para una de mis interfaces o si hay una coincidencia en mi tabla de enrutamiento 
  * - Si no es para una de mis interfaces y no hay coincidencia en la tabla de enrutamiento, enviar ICMP net unreachable
  * - Sino, si es para mí, verificar si es un paquete ICMP echo request y responder con un echo reply 
  * - Sino, verificar TTL, ARP y reenviar si corresponde (puede necesitar una solicitud ARP y esperar la respuesta)
  * - No olvide imprimir los mensajes de depuración
  */
  printf("Entro a handlepacket");

  sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t *)(sizeof(sr_ethernet_hdr_t) + packet);

  struct sr_if* interface_if = sr_get_interface_given_ip(sr, ip_hdr->ip_dst);

  if (interface_if == NULL) {

      ip_hdr->ip_ttl--;

      if (ip_hdr->ip_ttl == 0) {
          sr_send_icmp_error_packet(icmp_type_time_exceeded, 0, sr, ntohl(ip_hdr->ip_src), packet);
          return;
      }

      ip_hdr->ip_sum = cksum(ip_hdr, 4 * ip_hdr->ip_hl);

      struct sr_rt* best_rt = lpm(sr, ip_hdr->ip_dst);

      if (best_rt == NULL) {
          sr_send_icmp_error_packet(icmp_type_dest_unreachable, 0, sr, ntohl(ip_hdr->ip_src), packet);
          return;
      }

      uint32_t nh_ip = best_rt->gw.s_addr;
      if(nh_ip == 0){
        nh_ip = ip_hdr->ip_dst;
      }

      struct sr_if* out_interface = sr_get_interface(sr, best_rt->interface);

      struct sr_arpentry *arp_entry = sr_arpcache_lookup(&(sr->cache), nh_ip);

      if (arp_entry != NULL && arp_entry->valid) {
          printf("Entrada ARP encontrada, reenviando paquete\n");

          sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t *)packet;
          memcpy(eth_hdr->ether_dhost, arp_entry->mac, ETHER_ADDR_LEN);
          memcpy(eth_hdr->ether_shost, out_interface->addr, ETHER_ADDR_LEN);
          sr_send_packet(sr, packet, len, out_interface->name);

          free(arp_entry);

      } else {
          printf("No se encontró entrada ARP, enviando solicitud ARP\n");
          struct sr_arpreq* req = sr_arpcache_queuereq(&(sr->cache), nh_ip, packet, len, best_rt->interface);
          handle_arpreq(sr, req);
          printf("Solicitud ARP enviada\n");
      }      
  } else {
      /*sr_icmp_hdr_t* icmp_hdr = (sr_icmp_hdr_t*)((uint8_t *) sizeof(sr_ip_hdr_t) + packet);*/
      sr_icmp_hdr_t* icmp_hdr = (sr_icmp_hdr_t*)((uint8_t *)ip_hdr + (ip_hdr->ip_hl * 4)); /*accedo al header del ICMP*/ 
      printf("Llega aca");
      if (ip_protocol((uint8_t *) ip_hdr) == ip_protocol_icmp && icmp_hdr->icmp_type == icmp_echo_request){
        printf("ICMP echo request recibido.\n");
        
        uint8_t* pkt_icmp = generate_icmp_packet(icmp_echo_reply, 0, packet, sr, interface_if);
        unsigned int length_icmp = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
        sr_send_packet(sr, pkt_icmp, length_icmp, interface_if->name);

        /*sr_send_icmp_echo_reply(sr, packet, len, interface_if->name);*/

        printf("ICMP echo request respondido.\n");

      } else { 
        printf("Paquete que no se sabe que es\n");
				sr_send_icmp_error_packet(icmp_type_dest_unreachable, icmp_code_port_unreachable, sr, ip_hdr->ip_src, packet);
        printf("enviando ICMP a puerto inalcanzable\n");
      }

      printf("Salio de handlepacket");
  }


 /*Gracias Gepeto

 // Obtener el encabezado IP
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t *)(packet + sizeof(sr_ethernet_hdr_t)); //cabecera IP
    uint16_t length = ip_hdr->ip_len; // tamanio de la cabecera IP
    uint32_t* src_ip = ip_hdr->ip_src; // direccion origen IP 
    uint32_t* dest_ip = ip_hdr->ip_dst; // direccion destino IP
    
    // Verificar si el paquete es para una de mis interfaces
    struct sr_if *my_interface = sr_get_interface_by_ip(sr, ip_dst);
    if (my_interface) {
        // Paquete es para una de mis interfaces
        fprintf(stderr, "Paquete IP es para mí, procesando...\n");

        // Verificar si es un paquete ICMP
        if (ip_hdr->protocol == IPPROTO_ICMP) {
            struct icmphdr *icmp_hdr = (struct icmphdr *)(packet + (ip_hdr->ihl * 4));

            if (icmp_hdr->type == ICMP_ECHO) {
                // ICMP Echo request, responder con Echo reply
                fprintf(stderr, "ICMP Echo request recibido, respondiendo con Echo reply...\n");

                // Cambiar tipo de ICMP a Echo reply
                icmp_hdr->type = ICMP_ECHOREPLY;
                icmp_hdr->checksum = 0;  // Resetear antes de calcular
                icmp_hdr->checksum = ip_cksum(icmp_hdr, len - sizeof(struct iphdr));

                // Intercambiar las direcciones IP de origen y destino
                ip_hdr->daddr = ip_src;
                ip_hdr->saddr = ip_dst;
                
                // Recalcular el checksum IP
                ip_hdr->check = 0;
                ip_hdr->check = ip_cksum(ip_hdr, ip_hdr->ihl * 4);

                // Enviar respuesta ICMP Echo reply
                sr_send_packet(sr, packet, len, my_interface->name);
            }
        }
        return;
    }

    // Si no es para una de mis interfaces, buscar en la tabla de enrutamiento
    struct sr_rt *route = sr_find_route(sr, ip_dst);
    if (!route) {
        // No hay coincidencia en la tabla de enrutamiento, enviar ICMP Net Unreachable
        fprintf(stderr, "No hay ruta para el destino, enviando ICMP net unreachable\n");
        sr_send_icmp_error_packet(ICMP_DEST_UNREACH, ICMP_NET_UNREACH, sr, ip_src, packet);
        return;
    }

    // Verificar el TTL
    if (ip_hdr->ttl <= 1) {
        // TTL expirado, enviar ICMP Time Exceeded
        fprintf(stderr, "TTL expirado, enviando ICMP time exceeded\n");
        sr_send_icmp_error_packet(ICMP_TIME_EXCEEDED, 0, sr, ip_src, packet);
        return;
    }

    // Decrementar TTL y recalcular el checksum IP
    ip_hdr->ttl--;
    ip_hdr->check = 0;
    ip_hdr->check = ip_cksum(ip_hdr, ip_hdr->ihl * 4);

    // Verificar si tenemos la dirección MAC del siguiente salto en la tabla ARP
    struct sr_arpentry *arp_entry = sr_arpcache_lookup(&(sr->cache), route->gw.s_addr);
    if (arp_entry) {
        // Si tenemos la entrada ARP, reenviar el paquete
        fprintf(stderr, "Reenviando el paquete IP al siguiente salto\n");

        // Actualizar la dirección MAC destino
        memcpy(eHdr->ether_dhost, arp_entry->mac, ETHER_ADDR_LEN);

        // Actualizar la dirección MAC origen (nuestra dirección MAC)
        struct sr_if *out_iface = sr_get_interface(sr, route->interface);
        memcpy(eHdr->ether_shost, out_iface->addr, ETHER_ADDR_LEN);

        sr_send_packet(sr, packet, len, route->interface);
        free(arp_entry);
    } else {
        // Si no tenemos la dirección MAC, enviar una solicitud ARP
        fprintf(stderr, "Enviando solicitud ARP para obtener la MAC del siguiente salto\n");
        sr_arpcache_queuereq(&(sr->cache), route->gw.s_addr, packet, len, route->interface);
    }
  */
}

/* 
* ***** A partir de aquí no debería tener que modificar nada ****
*/

/* Envía todos los paquetes IP pendientes de una solicitud ARP */
void sr_arp_reply_send_pending_packets(struct sr_instance *sr,
                                        struct sr_arpreq *arpReq,
                                        uint8_t *dhost,
                                        uint8_t *shost,
                                        struct sr_if *iface) {

  struct sr_packet *currPacket = arpReq->packets;
  sr_ethernet_hdr_t *ethHdr;
  uint8_t *copyPacket;

  while (currPacket != NULL) {
     ethHdr = (sr_ethernet_hdr_t *) currPacket->buf;
     memcpy(ethHdr->ether_shost, dhost, sizeof(uint8_t) * ETHER_ADDR_LEN);
     memcpy(ethHdr->ether_dhost, shost, sizeof(uint8_t) * ETHER_ADDR_LEN);

     copyPacket = malloc(sizeof(uint8_t) * currPacket->len);
     memcpy(copyPacket, ethHdr, sizeof(uint8_t) * currPacket->len);

     print_hdrs(copyPacket, currPacket->len);
     sr_send_packet(sr, copyPacket, currPacket->len, iface->name);
     currPacket = currPacket->next;
  }
}

/* Gestiona la llegada de un paquete ARP*/
void sr_handle_arp_packet(struct sr_instance *sr,
        uint8_t *packet /* lent */,
        unsigned int len,
        uint8_t *srcAddr,
        uint8_t *destAddr,
        char *interface /* lent */,
        sr_ethernet_hdr_t *eHdr) {

  /* Imprimo el cabezal ARP */
  printf("*** -> It is an ARP packet. Print ARP header.\n");
  print_hdr_arp(packet + sizeof(sr_ethernet_hdr_t));

  /* Obtengo el cabezal ARP */
  sr_arp_hdr_t *arpHdr = (sr_arp_hdr_t *) (packet + sizeof(sr_ethernet_hdr_t));

  /* Obtengo las direcciones MAC */
  unsigned char senderHardAddr[ETHER_ADDR_LEN], targetHardAddr[ETHER_ADDR_LEN];
  memcpy(senderHardAddr, arpHdr->ar_sha, ETHER_ADDR_LEN);
  memcpy(targetHardAddr, arpHdr->ar_tha, ETHER_ADDR_LEN);

  /* Obtengo las direcciones IP */
  uint32_t senderIP = arpHdr->ar_sip;
  uint32_t targetIP = arpHdr->ar_tip;
  unsigned short op = ntohs(arpHdr->ar_op);

  /* Verifico si el paquete ARP es para una de mis interfaces */
  struct sr_if *myInterface = sr_get_interface_given_ip(sr, targetIP);

  if (op == arp_op_request) {  /* Si es un request ARP */
    printf("**** -> It is an ARP request.\n");

    /* Si el ARP request es para una de mis interfaces */
    if (myInterface != 0) {
      printf("***** -> ARP request is for one of my interfaces.\n");

      /* Agrego el mapeo MAC->IP del sender a mi caché ARP */
      printf("****** -> Add MAC->IP mapping of sender to my ARP cache.\n");
      sr_arpcache_insert(&(sr->cache), senderHardAddr, senderIP);

      /* Construyo un ARP reply y lo envío de vuelta */
      printf("****** -> Construct an ARP reply and send it back.\n");
      memcpy(eHdr->ether_shost, (uint8_t *) myInterface->addr, sizeof(uint8_t) * ETHER_ADDR_LEN);
      memcpy(eHdr->ether_dhost, (uint8_t *) senderHardAddr, sizeof(uint8_t) * ETHER_ADDR_LEN);
      memcpy(arpHdr->ar_sha, myInterface->addr, ETHER_ADDR_LEN);
      memcpy(arpHdr->ar_tha, senderHardAddr, ETHER_ADDR_LEN);
      arpHdr->ar_sip = targetIP;
      arpHdr->ar_tip = senderIP;
      arpHdr->ar_op = htons(arp_op_reply);

      /* Imprimo el cabezal del ARP reply creado */
      print_hdrs(packet, len);

      sr_send_packet(sr, packet, len, myInterface->name);
    }

    printf("******* -> ARP request processing complete.\n");

  } else if (op == arp_op_reply) {  /* Si es un reply ARP */

    printf("**** -> It is an ARP reply.\n");

    /* Agrego el mapeo MAC->IP del sender a mi caché ARP */
    printf("***** -> Add MAC->IP mapping of sender to my ARP cache.\n");
    struct sr_arpreq *arpReq = sr_arpcache_insert(&(sr->cache), senderHardAddr, senderIP);
    
    if (arpReq != NULL) { /* Si hay paquetes pendientes */

    	printf("****** -> Send outstanding packets.\n");
    	sr_arp_reply_send_pending_packets(sr, arpReq, (uint8_t *) myInterface->addr, (uint8_t *) senderHardAddr, myInterface);
    	sr_arpreq_destroy(&(sr->cache), arpReq);

    }
    printf("******* -> ARP reply processing complete.\n");
  }
}

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{
  assert(sr);
  assert(packet);
  assert(interface);

  printf("*** -> Received packet of length %d \n",len);

  /* Obtengo direcciones MAC origen y destino */
  sr_ethernet_hdr_t *eHdr = (sr_ethernet_hdr_t *) packet;
  uint8_t *destAddr = malloc(sizeof(uint8_t) * ETHER_ADDR_LEN);
  uint8_t *srcAddr = malloc(sizeof(uint8_t) * ETHER_ADDR_LEN);
  memcpy(destAddr, eHdr->ether_dhost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  memcpy(srcAddr, eHdr->ether_shost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  uint16_t pktType = ntohs(eHdr->ether_type);

  if (is_packet_valid(packet, len)) {
    if (pktType == ethertype_arp) {
      sr_handle_arp_packet(sr, packet, len, srcAddr, destAddr, interface, eHdr);
    } else if (pktType == ethertype_ip) {
      sr_handle_ip_packet(sr, packet, len, srcAddr, destAddr, interface, eHdr);
    }
  }

}/* end sr_ForwardPacket */