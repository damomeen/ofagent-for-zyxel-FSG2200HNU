/*
 * Distributed under the terms of the GNU GPL version 2.
 * Copyright (c) 2013 PSNC
 * Autor: damian.parniewicz at man.poznan.pl
 */

#ifndef ZYXEL_H
#define ZYXEL_H 1

#include "ofagent.h"

#define PORT_BANDWIDTH 10000000000

struct zyxel_service_params {
    uint16_t in_port;
    uint32_t wan_address;
    uint32_t wan_mask;
    short vlan_tag;
    short vlan_pcp;
    short flow_counter;
};

#define MAX_SERVICES 7
struct zyxel_properties {
    uint32_t wildcards;
    struct dev_action* supported_actions;
    int supported_actions_len;
    struct zyxel_service_params created_services[MAX_SERVICES];
    short srv_counter;
    short service_allocator_index;
    int ports_cnt;
};

extern struct zyxel_properties zyxel_props;

void zyxel_init();
int output_conf_zyxel(struct inserted_flow *infl, struct dev_properties *devprop);
int output_unconf_zyxel(struct inserted_flow *infl, struct dev_properties *devprop);
int zyxel_configure_service(uint32_t wan_address, uint32_t wan_mask, short vlan_tag, short vlan_pcp);
int zyxel_unconfigure_service(uint32_t wan_address, uint32_t wan_mask, short vlan_tag, short vlan_pcp);

#endif /* ZYXEL_H */