#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sr_protocol.h"
#include "pwospf_protocol.h"
#include "sr_utils.h"


uint16_t cksum (const void *_data, int len) {
  const uint8_t *data = _data;
  uint32_t sum;

  for (sum = 0;len >= 2; data += 2, len -= 2)
    sum += data[0] << 8 | data[1];
  if (len > 0)
    sum += data[0] << 8;
  while (sum > 0xffff)
    sum = (sum >> 16) + (sum & 0xffff);
  sum = htons (~sum);
  return sum ? sum : 0xffff;
}

uint32_t ip_cksum (sr_ip_hdr_t *ipHdr, int len) {
    uint16_t currChksum, calcChksum;

    currChksum = ipHdr->ip_sum; 
    ipHdr->ip_sum = 0;
    calcChksum = cksum(ipHdr, len);
    ipHdr->ip_sum = currChksum;    

    return calcChksum;
}

uint32_t icmp_cksum (sr_icmp_hdr_t *icmpHdr, int len) {
    uint16_t currChksum, calcChksum;

    currChksum = icmpHdr->icmp_sum; 
    icmpHdr->icmp_sum = 0;
    calcChksum = cksum(icmpHdr, len);
    icmpHdr->icmp_sum = currChksum;

    return calcChksum;
}

uint32_t icmp3_cksum(sr_icmp_t3_hdr_t *icmp3Hdr, int len) {
    uint16_t currChksum, calcChksum;

    currChksum = icmp3Hdr->icmp_sum;
    icmp3Hdr->icmp_sum = 0;
    calcChksum = cksum(icmp3Hdr, len);
    icmp3Hdr->icmp_sum = currChksum;
    
    return calcChksum;
}

uint32_t ospfv2_cksum (ospfv2_hdr_t *ospfv2Hdr, int len) {
    uint16_t currChksum, calcChksum;

    currChksum = ospfv2Hdr->csum; 
    ospfv2Hdr->csum = 0;
    calcChksum = cksum(ospfv2Hdr, len);
    ospfv2Hdr->csum = currChksum;    

    return calcChksum;
}

int is_packet_valid(uint8_t *packet /* lent */,
    unsigned int len) {

  int cumulative_sz = sizeof(sr_ethernet_hdr_t);
  sr_ethernet_hdr_t *eHdr = (sr_ethernet_hdr_t *) packet;
    
  printf("*** -> Perform validation on the packet.\n");

  if (eHdr->ether_type == htons(ethertype_arp)) {
    printf("**** -> Validate ARP packet.\n");
    cumulative_sz += sizeof(sr_arp_hdr_t);
    if (len >= cumulative_sz) {
      printf("***** -> Packet length is correct.\n");
      return 1;
    }
  } else if (eHdr->ether_type == htons(ethertype_ip)) {
    printf("**** -> Validate IP packet.\n");
    sr_ip_hdr_t *ipHdr = (sr_ip_hdr_t *) (packet + cumulative_sz);
    cumulative_sz += sizeof(sr_ip_hdr_t);

    if (len >= cumulative_sz) {
      printf("***** -> Packet length is correct.\n");
      if (ip_cksum(ipHdr, sizeof(sr_ip_hdr_t)) == ipHdr->ip_sum) {
        printf("***** -> IP packet checksum is correct.\n");
        if (ipHdr->ip_p == ip_protocol_icmp) {
          printf("***** -> IP packet is ICMP packet. Validate ICMP packet.\n");
          int icmpOffset = cumulative_sz;
          sr_icmp_hdr_t *icmpHdr = (sr_icmp_hdr_t *) (packet + cumulative_sz);
          cumulative_sz += sizeof(sr_icmp_hdr_t);

          if (len >= cumulative_sz) {
            printf("****** -> Packet length is correct.\n");
            if (icmp_cksum(icmpHdr, len - icmpOffset) == icmpHdr->icmp_sum) {
              printf("****** -> ICMP packet checksum is correct.\n");
              return 1;
            }
          }
        } else if (ipHdr->ip_p == ip_protocol_ospfv2) {
          printf("***** -> IP packet is OSPF packet. Validate OSPF packet.\n");
          int ospfOffset = cumulative_sz;
          ospfv2_hdr_t *ospfHdr = (ospfv2_hdr_t *) (packet + cumulative_sz);
          cumulative_sz += sizeof(ospfv2_hdr_t);

          if (len >= cumulative_sz) {
            printf("****** -> Packet length is correct.\n");
            if (ospfv2_cksum(ospfHdr, len - ospfOffset) == ospfHdr->csum) {
              printf("****** -> OSPF packet checksum is correct.\n");
              return 1;
            }
          }
        } else {
          /* TODO */
          printf("***** -> IP packet is of unknown protocol. No further validation is required.\n");
	        return 1;
        }
      }
    }
  }
  printf("*** -> Packet validation complete. Packet is INVALID.\n");
  return 0;
}

/* Helper function for sr_arp_request_send to generate
   broadcast MAC address. */ 
uint8_t *generate_ethernet_addr(uint8_t x) {
    uint8_t *mac = malloc(sizeof(uint8_t) * ETHER_ADDR_LEN);
    int i;
    for (i = 0; i < ETHER_ADDR_LEN; i++) {
        mac[i] = x;
    }
    return mac;
}

uint16_t ethertype(uint8_t *buf) {
  sr_ethernet_hdr_t *ehdr = (sr_ethernet_hdr_t *)buf;
  return ntohs(ehdr->ether_type);
}

uint8_t ip_protocol(uint8_t *buf) {
  sr_ip_hdr_t *iphdr = (sr_ip_hdr_t *)(buf);
  return iphdr->ip_p;
}


/* Prints out formatted Ethernet address, e.g. 00:11:22:33:44:55 */
void print_addr_eth(uint8_t *addr) {
  int pos = 0;
  uint8_t cur;
  for (; pos < ETHER_ADDR_LEN; pos++) {
    cur = addr[pos];
    if (pos > 0)
      fprintf(stderr, ":");
    fprintf(stderr, "%02X", cur);
  }
  fprintf(stderr, "\n");
}

/* Prints out IP address as a string from in_addr */
void print_addr_ip(struct in_addr address) {
  char buf[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &address, buf, 100) == NULL)
    fprintf(stderr,"inet_ntop error on address conversion\n");
  else
    fprintf(stderr, "%s\n", buf);
}

/* Prints out IP address from integer value */
void print_addr_ip_int(uint32_t ip) {
  uint32_t curOctet = ip >> 24;
  fprintf(stderr, "%d.", curOctet);
  curOctet = (ip << 8) >> 24;
  fprintf(stderr, "%d.", curOctet);
  curOctet = (ip << 16) >> 24;
  fprintf(stderr, "%d.", curOctet);
  curOctet = (ip << 24) >> 24;
  fprintf(stderr, "%d\n", curOctet);
}


/* Prints out fields in Ethernet header. */
void print_hdr_eth(uint8_t *buf) {
  sr_ethernet_hdr_t *ehdr = (sr_ethernet_hdr_t *)buf;
  fprintf(stderr, "ETHERNET header:\n");
  fprintf(stderr, "\tdestination: ");
  print_addr_eth(ehdr->ether_dhost);
  fprintf(stderr, "\tsource: ");
  print_addr_eth(ehdr->ether_shost);
  fprintf(stderr, "\ttype: %d\n", ntohs(ehdr->ether_type));
}

/* Prints out fields in IP header. */
void print_hdr_ip(uint8_t *buf) {
  sr_ip_hdr_t *iphdr = (sr_ip_hdr_t *)(buf);
  fprintf(stderr, "IP header:\n");
  fprintf(stderr, "\tversion: %d\n", iphdr->ip_v);
  fprintf(stderr, "\theader length: %d\n", iphdr->ip_hl);
  fprintf(stderr, "\ttype of service: %d\n", iphdr->ip_tos);
  fprintf(stderr, "\tlength: %d\n", ntohs(iphdr->ip_len));
  fprintf(stderr, "\tid: %d\n", ntohs(iphdr->ip_id));

  if (ntohs(iphdr->ip_off) & IP_DF)
    fprintf(stderr, "\tfragment flag: DF\n");
  else if (ntohs(iphdr->ip_off) & IP_MF)
    fprintf(stderr, "\tfragment flag: MF\n");
  else if (ntohs(iphdr->ip_off) & IP_RF)
    fprintf(stderr, "\tfragment flag: R\n");

  fprintf(stderr, "\tfragment offset: %d\n", ntohs(iphdr->ip_off) & IP_OFFMASK);
  fprintf(stderr, "\tTTL: %d\n", iphdr->ip_ttl);
  fprintf(stderr, "\tprotocol: %d\n", iphdr->ip_p);

  /*Keep checksum in NBO*/
  fprintf(stderr, "\tchecksum: %d\n", iphdr->ip_sum);

  fprintf(stderr, "\tsource: ");
  print_addr_ip_int(ntohl(iphdr->ip_src));

  fprintf(stderr, "\tdestination: ");
  print_addr_ip_int(ntohl(iphdr->ip_dst));
}

/* Prints out ICMP header fields */
void print_hdr_icmp(uint8_t *buf) {
  sr_icmp_hdr_t *icmp_hdr = (sr_icmp_hdr_t *)(buf);
  fprintf(stderr, "ICMP header:\n");
  fprintf(stderr, "\ttype: %d\n", icmp_hdr->icmp_type);
  fprintf(stderr, "\tcode: %d\n", icmp_hdr->icmp_code);
  /* Keep checksum in NBO */
  fprintf(stderr, "\tchecksum: %d\n", icmp_hdr->icmp_sum);
}


/* Prints out fields in ARP header */
void print_hdr_arp(uint8_t *buf) {
  sr_arp_hdr_t *arp_hdr = (sr_arp_hdr_t *)(buf);
  fprintf(stderr, "ARP header\n");
  fprintf(stderr, "\thardware type: %d\n", ntohs(arp_hdr->ar_hrd));
  fprintf(stderr, "\tprotocol type: %d\n", ntohs(arp_hdr->ar_pro));
  fprintf(stderr, "\thardware address length: %d\n", arp_hdr->ar_hln);
  fprintf(stderr, "\tprotocol address length: %d\n", arp_hdr->ar_pln);
  fprintf(stderr, "\topcode: %d\n", ntohs(arp_hdr->ar_op));

  fprintf(stderr, "\tsender hardware address: ");
  print_addr_eth(arp_hdr->ar_sha);
  fprintf(stderr, "\tsender ip address: ");
  print_addr_ip_int(ntohl(arp_hdr->ar_sip));

  fprintf(stderr, "\ttarget hardware address: ");
  print_addr_eth(arp_hdr->ar_tha);
  fprintf(stderr, "\ttarget ip address: ");
  print_addr_ip_int(ntohl(arp_hdr->ar_tip));
}

/*Prints out fields in PWOSPF header */
void print_hdr_ospf(uint8_t* buf) {
  ospfv2_hdr_t *ospf_hdr = (ospfv2_hdr_t *)(buf);
  fprintf(stderr, "PWOSPF header\n");
  fprintf(stderr, "\tversion: %d\n", ospf_hdr->version);
  fprintf(stderr, "\ttype: %d\n", ospf_hdr->type);
  fprintf(stderr, "\tlength: %d\n", ntohs(ospf_hdr->len));
  fprintf(stderr, "\trouter id: ");
  print_addr_ip_int(ntohl(ospf_hdr->rid));
  fprintf(stderr, "\tarea id: ");
  print_addr_ip_int(ntohl(ospf_hdr->aid));
  fprintf(stderr, "\tchecksum: %d\n", ospf_hdr->csum);
  fprintf(stderr, "\tauthentication type: %d\n", ospf_hdr->autype);
  /* Actualizado */
  fprintf(stderr, "\tauthentication data: %ld\n", ospf_hdr->audata);
}

/* Prints out all possible headers, starting from Ethernet */
void print_hdrs(uint8_t *buf, uint32_t length) {

  /* Ethernet */
  int minlength = sizeof(sr_ethernet_hdr_t);
  if (length < minlength) {
    fprintf(stderr, "Failed to print ETHERNET header, insufficient length\n");
    return;
  }

  uint16_t ethtype = ethertype(buf);
  print_hdr_eth(buf);

  if (ethtype == ethertype_ip) { /* IP */
    minlength += sizeof(sr_ip_hdr_t);
    if (length < minlength) {
      fprintf(stderr, "Failed to print IP header, insufficient length\n");
      return;
    }

    print_hdr_ip(buf + sizeof(sr_ethernet_hdr_t));
    uint8_t ip_proto = ip_protocol(buf + sizeof(sr_ethernet_hdr_t));

    if (ip_proto == ip_protocol_icmp) { /* ICMP */
      minlength += sizeof(sr_icmp_hdr_t);
      if (length < minlength)
        fprintf(stderr, "Failed to print ICMP header, insufficient length\n");
      else
        print_hdr_icmp(buf + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
    }
  }
  else if (ethtype == ethertype_arp) { /* ARP */
    minlength += sizeof(sr_arp_hdr_t);
    if (length < minlength)
      fprintf(stderr, "Failed to print ARP header, insufficient length\n");
    else
      print_hdr_arp(buf + sizeof(sr_ethernet_hdr_t));
  }
  else {
    fprintf(stderr, "Unrecognized Ethernet Type: %d\n", ethtype);
  }
}

