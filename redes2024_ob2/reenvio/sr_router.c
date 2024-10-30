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
/* Agregados */
#include "sr_icmp.h"

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Inicializa el subsistema de enrutamiento
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance *sr)
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

/* Funcion auxiliar Longest Prefix Match (lpm). Permite encontrar en la tabla de enrutamiento la entrada con la coincidencia
  mas larga para la IP de destino */
struct sr_rt *lpm(struct sr_instance *sr, uint32_t dest_ip)
{
  /* Tomo el puntero a la tabla de enrutamiento de la instancia del router */
  struct sr_rt *rt_iterator = sr->routing_table;
  /* Inicializo Best Prefix Match (bpm), que sera el que vaya almacenando el prefijo mas largo encontrado hasta el momento */
  struct sr_rt *bpm = NULL;
  while (rt_iterator != NULL)
  {
    /* Si la IP destino al ser enmascarada coincide con la IP destino de la entrada de la tabla */
    if ((dest_ip & rt_iterator->mask.s_addr) == rt_iterator->dest.s_addr)
    {
      /* Y la mascara de la entrada es mayor a la de la mejor entrada hasta el momento (por ende el prefijo es mas largo) */
      if (bpm == NULL || rt_iterator->mask.s_addr > bpm->mask.s_addr)
      {
        /* Almaceno esa entrada en bpm */
        bpm = rt_iterator;
      }
    }
    /* Avanzo en el iterador */
    rt_iterator = rt_iterator->next;
  }
  return bpm;
}

/* Envía un paquete ICMP de respuesta echo */
void sr_send_icmp_echo_reply(struct sr_instance *sr,
                             uint32_t ipDst,
                             uint8_t *ipPacket)
{
  /* COLOQUE AQUÍ SU CÓDIGO*/
  printf("****** -> Construct ICMP echo reply.\n");
  /* Obtengo la interfaz a la que mandar el paquete a partir de la tabla de enrutamiento */
  char *target_interface_name = lpm(sr, ipDst)->interface;
  printf("****** -> ICMP reply targets interface: ");
  printf("%s\n", target_interface_name);
  struct sr_if *target_interface = sr_get_interface(sr, target_interface_name);

  /* Genero el paquete ICMP, calculo su tamanio y lo envio*/
  uint8_t *icmp_packet = generate_icmp_packet(icmp_echo_reply, 0, ipPacket, sr, target_interface);
  /* Tomo el largo del paquete creado, pues largo variable con el payload ICMP */
  unsigned int ip_len = ntohs(((sr_ip_hdr_t *)(icmp_packet + sizeof(sr_ethernet_hdr_t)))->ip_len);
  unsigned int icmp_len = sizeof(sr_ethernet_hdr_t) + ip_len;
  printf("****** -> ICMP echo reply headers:\n");
  print_hdrs(icmp_packet, icmp_len);
  sr_send_packet(sr, icmp_packet, icmp_len, target_interface_name);
  printf("****** -> ICMP echo reply sent.\n");
  /* Libero la memoria asociada REVISAR */
  delete_icmp_packet(icmp_packet);
  printf("****** -> ICMP reply end.\n");

} /* -- sr_send_icmp_echo_reply -- */

/* Envía un paquete ICMP de error */
void sr_send_icmp_error_packet(uint8_t type,
                               uint8_t code,
                               struct sr_instance *sr,
                               uint32_t ipDst,
                               uint8_t *ipPacket)
{

  /* COLOQUE AQUÍ SU CÓDIGO*/
  printf("***** -> Construct ICMP error response.\n");

  /* Obtengo la interfaz a la que mandar el paquete a partir de la tabla de enrutamiento */
  char *target_interface_name = lpm(sr, ipDst)->interface;
  printf("****** -> ICMP error response targets interface: ");
  printf("%s\n", target_interface_name);
  struct sr_if *target_interface = sr_get_interface(sr, target_interface_name);

  /* Genero el paquete ICMP, calculo su tamanio y lo envio*/
  uint8_t *icmp_t3_packet = generate_icmp_t3_packet(type, code, ipPacket, sr, target_interface);
  unsigned int icmp_len = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_t3_hdr_t);
  printf("****** -> ICMP error response headers:\n");
  print_hdrs(icmp_t3_packet, icmp_len);
  sr_send_packet(sr, icmp_t3_packet, icmp_len, target_interface_name);
  printf("****** -> ICMP error response sent.\n");
  /* Libero la memoria asociada REVISAR */
  delete_icmp_t3_packet(icmp_t3_packet);
  printf("****** -> ICMP error response end.\n");

} /* -- sr_send_icmp_error_packet -- */

void sr_handle_ip_packet(struct sr_instance *sr,
                         uint8_t *packet /* lent */,
                         unsigned int len,
                         uint8_t *srcAddr,
                         uint8_t *destAddr,
                         char *interface /* lent */,
                         sr_ethernet_hdr_t *eHdr)
{

  /*
   * COLOQUE ASÍ SU CÓDIGO
   * SUGERENCIAS:
   * - No olvide imprimir los mensajes de depuración
   */
  print_hdrs(packet, len);

  /* Obtengo el cabezal IP */
  /* packet es un puntero a la posicion inicial del paquete en el buffer, debo indicar la posicion inicial de la
    cabecera IP. La cabecera IP comienza inmediatamente despues que termina la cabecera Ethernet. */
  sr_ip_hdr_t *ip_hdr = (sr_ip_hdr_t *)(packet + sizeof(sr_ethernet_hdr_t));

  /* La validacion de la sanidad del paquete ya viene hecha desde la funcion handle_packet (Ethernet e IP) */

  /* Imprimo el cabezal IP */
  printf("*** -> It is an IP packet. Print IP header.\n");
  print_hdr_ip(packet + sizeof(sr_ethernet_hdr_t));

  /* Obtengo las direcciones IP */
  uint32_t sender_IP = ip_hdr->ip_src;
  uint32_t target_IP = ip_hdr->ip_dst;

  /* Verifico si paquete para una de mis interfaces */
  struct sr_if *target_interface = sr_get_interface_given_ip(sr, target_IP);
  printf("*** -> IP request targets interface: ");

  /* Si no es para una de mis interfaces (sr_get_interface_given_ip retorna 0 porque no encontro la interfaz
    en la lista de interfaces del router) */
  if (target_interface == 0)
  {
    printf("Not found\n");

    printf("**** -> IP request is not for one of my interfaces.\n");

    /* Decremento TTL y calculo checksum nuevamente */
    ip_hdr->ip_ttl--;
    ip_hdr->ip_sum = ip_cksum(ip_hdr, 4 * ip_hdr->ip_hl);

    /* Si TTL > 0, proceso el paquete para un reenvio */
    if (ip_hdr->ip_ttl > 0)
    {
      /* Verifico si hay una coincidencia en mi tabla de enrutamiento */
      struct sr_rt *best_rt = lpm(sr, target_IP);
      /* Si no hay coincidencia */
      if (best_rt == NULL)
      {
        /* No es para una de mis interfaces y no hay coincidencia en la tabla de enrutamiento */
        /* Responder con error: Tipo 3, Codigo 0 : Red no alcanzable */
        printf("***** -> IP request is for unknown destiny.\n");
        sr_send_icmp_error_packet(icmp_type_dest_unreachable, icmp_code_net_unreachable, sr, sender_IP, packet);
      }
      /* Si hay coincidencia */
      else
      {
        /* Verificar ARP y reenviar si corresponde (puede necesitar una solicitud ARP y esperar la respuesta) */
        /* Obtengo la IP del proximo salto (gate away) de la tabla de enrutamiento */
        uint32_t next_hop_ip = best_rt->gw.s_addr;

        /* Busca en la ARP cache si ya hay una direccion MAC para la IP del proximo salto */
        struct sr_arpentry *arp_entry = sr_arpcache_lookup(&(sr->cache), next_hop_ip);

        /* Si la entrada no es nula y es valida, reenvio el paquete */
        if (arp_entry != NULL && arp_entry->valid)
        {
          printf("***** -> Next hop IP is in ARP cache.\n");
          /* Armo la cabecera Ethernet (la cabecera IP se mantiene igual) */
          sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t *)packet;
          /* Sobreescribimos el paquete recibido, cambiando las direcciones MAC */
          memcpy(eth_hdr->ether_dhost, arp_entry->mac, ETHER_ADDR_LEN);
          memcpy(eth_hdr->ether_shost, target_interface->addr, ETHER_ADDR_LEN);
          /* Envia el paquete Ethernet */
          sr_send_packet(sr, packet, len, target_interface->name);
          printf("***** -> Ethernet packet sent.\n");
          /* Libera la memoria del paquete (Es el mismo que se recibio en un principio) */
          free(arp_entry);
        }
        /* Si no se encontro una entrada para la IP del proximo salto en la cache ARP */
        else
        {
          printf("***** -> Next hop IP is not in ARP cache.\n");
          struct sr_arpreq *req = sr_arpcache_queuereq(&(sr->cache), next_hop_ip, packet, len, best_rt->interface);
          printf("***** -> Handle ARP request.\n");
          handle_arpreq(sr, req);
        }
      }
    }
    /* Si TTL = 0, tengo que responder con ICMP Time Exceeded: Tipo 11, Codigo 0 */
    else
    {
      sr_send_icmp_error_packet(icmp_type_time_exceeded, 0, sr, sender_IP, packet);
    }
  }
  /* Si es para una de mis interfaces */
  else
  {
    printf("%s\n", target_interface->name);

    printf("**** -> IP request is for one of my interfaces.\n");
    /* Verificar si es un paquete ICMP */
    if (ip_hdr->ip_p == ip_protocol_icmp)
    {
      /* Tomo la cabecera ICMP como el puntero a la cabecera IP mas el lago de esta x4, pues el tamanio
       en la cabecera IP se seniala en palabras de 32 bits (4 bytes)*/
      sr_icmp_hdr_t *icmp_hdr = (sr_icmp_hdr_t *)((u_int8_t *)ip_hdr + (ip_hdr->ip_hl * 4));

      printf("***** -> It is an ICMP packet. Print ICMP header.\n");
      print_hdr_icmp((u_int8_t *)icmp_hdr);

      /* Verificar suma de comprobacion ICMP */

      /* Si es un echo request*/
      if (icmp_hdr->icmp_type == icmp_echo_request)
      {
        printf("****** -> It is an ICMP echo request.\n");
        /* Responder con un echo reply : Tipo 0, Codigo 0*/
        sr_send_icmp_echo_reply(sr, sender_IP, packet);
      }
      /* Si no es un echo request */
      else
      {
        /* Responder con error: Tipo 3, Codigo ?. QUE ERROR SE ENVIA EN ESTE CASO */
        sr_send_icmp_error_packet(icmp_type_dest_unreachable, -1, sr, sender_IP, packet);
      }
    }
    /* Si no es un paquete ICMP (supongo que es TCP o UDP) */
    else
    {
      /* Responder con error: Tipo 3, Codigo 3 : Puerto no alcanzable */
      sr_send_icmp_error_packet(icmp_type_dest_unreachable, icmp_code_host_unreachable, sr, sender_IP, packet);
    }
  }
}

/*
 * ***** A partir de aquí no debería tener que modificar nada ****
 */

/* Envía todos los paquetes IP pendientes de una solicitud ARP */
void sr_arp_reply_send_pending_packets(struct sr_instance *sr,
                                       struct sr_arpreq *arpReq,
                                       uint8_t *dhost,
                                       uint8_t *shost,
                                       struct sr_if *iface)
{

  struct sr_packet *currPacket = arpReq->packets;
  sr_ethernet_hdr_t *ethHdr;
  uint8_t *copyPacket;

  while (currPacket != NULL)
  {
    ethHdr = (sr_ethernet_hdr_t *)currPacket->buf;
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
                          sr_ethernet_hdr_t *eHdr)
{

  /* Imprimo el cabezal ARP */
  printf("*** -> It is an ARP packet. Print ARP header.\n");
  print_hdr_arp(packet + sizeof(sr_ethernet_hdr_t));

  /* Obtengo el cabezal ARP */
  sr_arp_hdr_t *arpHdr = (sr_arp_hdr_t *)(packet + sizeof(sr_ethernet_hdr_t));

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

  if (op == arp_op_request)
  { /* Si es un request ARP */
    printf("**** -> It is an ARP request.\n");

    /* Si el ARP request es para una de mis interfaces */
    if (myInterface != 0)
    {
      printf("***** -> ARP request is for one of my interfaces.\n");

      /* Agrego el mapeo MAC->IP del sender a mi caché ARP */
      printf("****** -> Add MAC->IP mapping of sender to my ARP cache.\n");
      sr_arpcache_insert(&(sr->cache), senderHardAddr, senderIP);

      /* Construyo un ARP reply y lo envío de vuelta */
      printf("****** -> Construct an ARP reply and send it back.\n");
      memcpy(eHdr->ether_shost, (uint8_t *)myInterface->addr, sizeof(uint8_t) * ETHER_ADDR_LEN);
      memcpy(eHdr->ether_dhost, (uint8_t *)senderHardAddr, sizeof(uint8_t) * ETHER_ADDR_LEN);
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
  }
  else if (op == arp_op_reply)
  { /* Si es un reply ARP */

    printf("**** -> It is an ARP reply.\n");

    /* Agrego el mapeo MAC->IP del sender a mi caché ARP */
    printf("***** -> Add MAC->IP mapping of sender to my ARP cache.\n");
    struct sr_arpreq *arpReq = sr_arpcache_insert(&(sr->cache), senderHardAddr, senderIP);

    if (arpReq != NULL)
    { /* Si hay paquetes pendientes */

      printf("****** -> Send outstanding packets.\n");
      sr_arp_reply_send_pending_packets(sr, arpReq, (uint8_t *)myInterface->addr, (uint8_t *)senderHardAddr, myInterface);
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

void sr_handlepacket(struct sr_instance *sr,
                     uint8_t *packet /* lent */,
                     unsigned int len,
                     char *interface /* lent */)
{
  assert(sr);
  assert(packet);
  assert(interface);

  printf("*** -> Received packet of length %d \n", len);

  /* Obtengo direcciones MAC origen y destino */
  sr_ethernet_hdr_t *eHdr = (sr_ethernet_hdr_t *)packet;
  uint8_t *destAddr = malloc(sizeof(uint8_t) * ETHER_ADDR_LEN);
  uint8_t *srcAddr = malloc(sizeof(uint8_t) * ETHER_ADDR_LEN);
  memcpy(destAddr, eHdr->ether_dhost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  memcpy(srcAddr, eHdr->ether_shost, sizeof(uint8_t) * ETHER_ADDR_LEN);
  uint16_t pktType = ntohs(eHdr->ether_type);

  if (is_packet_valid(packet, len))
  {
    if (pktType == ethertype_arp)
    {
      sr_handle_arp_packet(sr, packet, len, srcAddr, destAddr, interface, eHdr);
    }
    else if (pktType == ethertype_ip)
    {
      sr_handle_ip_packet(sr, packet, len, srcAddr, destAddr, interface, eHdr);
    }
  }

} /* end sr_ForwardPacket */