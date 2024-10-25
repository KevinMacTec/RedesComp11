#include "sr_icmp.h"

static uint16_t ip_id_counter = 0;

uint8_t *generate_icmp_packet(uint8_t type,                  /* Nº de tipo de mensaje ICMP */
                            uint8_t code,                   /* Código dentro del tipo ICMP */
                            uint8_t* packet,                /* Puntero al paquete recibido */
                            struct sr_instance* sr,         /* Puntero a la instancia del router */
                            struct sr_if* interface){       /* Puntero a la interface  */

    /* ETHERNET */
    sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t*) (packet);

    /* MAC directions */
    uint8_t* src_MAC = eth_hdr->ether_dhost; 
    uint8_t* dest_MAC = eth_hdr->ether_shost;

    /* INTERNET PROTOCOL */
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t *)(sizeof(sr_ethernet_hdr_t) + packet);
    uint32_t src_ip = interface->ip;
    uint32_t dest_ip = ip_hdr->ip_src;
    
    uint8_t* packet_icmp = malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
    sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t*) (packet_icmp);

    /* ETHERNET HEADER */
    memcpy(new_eth_hdr->ether_shost, src_MAC, sizeof(uint8_t) * ETHER_ADDR_LEN);
    memcpy(new_eth_hdr->ether_dhost, dest_MAC, sizeof(uint8_t) * ETHER_ADDR_LEN);
    new_eth_hdr->ether_type = ethertype_ip;

    /* IP HEADER */
    sr_ip_hdr_t* new_ip_hdr = (sr_ip_hdr_t *)(sizeof(sr_ethernet_hdr_t) + packet);
    memcpy(new_ip_hdr, ip_hdr, sizeof(sr_ip_hdr_t));
    new_ip_hdr->ip_tos = 0;
    new_ip_hdr->ip_ttl = 16;
    new_ip_hdr->ip_hl = 5;
    new_ip_hdr->ip_v = 4;
    new_ip_hdr->ip_len = sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
    new_ip_hdr->ip_p = ip_protocol_icmp;
    new_ip_hdr->ip_src = src_ip;
    new_ip_hdr->ip_dst = dest_ip;
    new_ip_hdr->ip_id = ip_id_counter++;
    new_ip_hdr->ip_off = 0;
    new_ip_hdr->ip_sum = ip_cksum(new_ip_hdr, 4 * new_ip_hdr->ip_hl);

    /* ICMP HEADER */
    sr_icmp_hdr_t *new_icmp_hdr = (sr_icmp_hdr_t *)(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + packet);
    new_icmp_hdr->icmp_code = code;
    new_icmp_hdr->icmp_type = type;
    new_icmp_hdr->icmp_sum = icmp_cksum(new_icmp_hdr, sizeof(sr_icmp_hdr_t));

    return packet_icmp;
}

uint8_t *generate_icmp_packet_t3(uint8_t type,                  /* Nº de tipo de mensaje ICMP */
                                uint8_t code,                   /* Código dentro del tipo ICMP */
                                uint8_t* packet,                /* Puntero al paquete recibido */
                                struct sr_instance* sr,         /* Puntero a la instancia del router */
                                struct sr_if* interface){       /* Puntero a la interface  */

    /* ETHERNET */
    sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t*) (packet);

    /* MAC directions (hay que invertirlas) */
    uint8_t* src_MAC = eth_hdr->ether_dhost; 
    uint8_t* dest_MAC = eth_hdr->ether_shost;

    /* INTERNET PROTOCOL */
    sr_ip_hdr_t* ip_hdr = (sr_ip_hdr_t *)(sizeof(sr_ethernet_hdr_t) + packet);
    uint32_t src_ip = interface->ip;
    uint32_t dest_ip = ip_hdr->ip_src;
    
    uint8_t* packet_icmp = malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_t3_hdr_t));
    sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t*) (packet_icmp);
    
    /* ETHERNET HEADER */
    memcpy(new_eth_hdr->ether_shost, src_MAC, sizeof(uint8_t) * ETHER_ADDR_LEN);
    memcpy(new_eth_hdr->ether_dhost, dest_MAC, sizeof(uint8_t) * ETHER_ADDR_LEN);
    new_eth_hdr->ether_type = ethertype_ip;

    /* IP HEADER */
    sr_ip_hdr_t* new_ip_hdr = (sr_ip_hdr_t *)(sizeof(sr_ethernet_hdr_t) + packet);
    memcpy(new_ip_hdr, ip_hdr, sizeof(sr_ip_hdr_t));
    new_ip_hdr->ip_tos = 0;
    new_ip_hdr->ip_ttl = 16;
    new_ip_hdr->ip_hl = 5;
    new_ip_hdr->ip_v = 4;
    new_ip_hdr->ip_len = sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_t3_hdr_t);
    new_ip_hdr->ip_p = ip_protocol_icmp;
    new_ip_hdr->ip_src = src_ip;
    new_ip_hdr->ip_dst = dest_ip;
    new_ip_hdr->ip_id = ip_id_counter++;
    new_ip_hdr->ip_off = 0;
    new_ip_hdr->ip_sum = ip_cksum(new_ip_hdr, 4 * new_ip_hdr->ip_hl);
    
    /* ICMP HEADER */
    sr_icmp_t3_hdr_t *new_icmp_hdr = (sr_icmp_t3_hdr_t *)(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + packet);
    new_icmp_hdr->icmp_code = code;
    new_icmp_hdr->icmp_type = type;
    new_icmp_hdr->icmp_sum = icmp3_cksum(new_icmp_hdr, sizeof(sr_icmp_t3_hdr_t));
    /* CAMPOS DE TIPO 3 */
    new_icmp_hdr->unused = 0;
    new_icmp_hdr->next_mtu = 0;
    memcpy(new_icmp_hdr->data, ip_hdr, (4 * ip_hdr->ip_hl) + 8);
    
    return packet_icmp;
}