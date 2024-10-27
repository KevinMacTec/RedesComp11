#include "sr_icmp.h"

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
    /* free(icmp_data); */
    free(icmp_packet);
}

void delete_icmp_t3_packet(uint8_t* icmp_t3_packet){
    free(icmp_t3_packet);
}