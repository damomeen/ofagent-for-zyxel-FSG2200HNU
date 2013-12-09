 /*
 * Copyright (c) 2013 PSNC
 * Autors: Szymon Kuc, Damian Parniewicz
 */
 
#ifndef MILEGATE_H
#define MILEGATE_H 1

#include "ofagent.h"
#include "hw-layer/libtelnet.h"


struct milegate_port_info {
    unsigned short unit_no;
    unsigned short port_no;
    //char[30] port_desc;
};

struct service_params {
    uint16_t in_port;
    uint16_t c_tag;
    short flow_counter;
    unsigned short interfaceID;
    unsigned short serviceID;
};

#define MAX_SERVICES 1000
struct milegate_properties {
    uint32_t wildcards;
    struct dev_action* supported_actions;
    int supported_actions_len;
    struct milegate_port_info of_to_milegate_portmap[50];
    //short int pcpToTcMappingTable[10][8];
    struct service_params created_services[MAX_SERVICES];
    short srv_counter;
    short service_allocator_index;
    char *login;
    char *passwd;
    char *ip_addr;
    char *telnet_port;
};

struct milegate_conf {
    unsigned short interfaceID;
    unsigned short serviceID;
    //struct milegate_port_info portID;
    unsigned short unit_no;
    unsigned short port_no;
};

extern struct milegate_properties milegate_props;

#endif /* MILEGATE_H */