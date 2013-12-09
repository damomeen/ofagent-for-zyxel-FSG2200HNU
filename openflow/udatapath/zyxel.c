/*
 * Distributed under the terms of the GNU GPL version 2.
 * Copyright (c) 2013 PSNC
 * Autor: damian.parniewicz at man.poznan.pl
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "zyxel.h"

#include "vlog.h"
#define THIS_MODULE VLM_datapath

struct zyxel_properties zyxel_props;

char port_number[10];

extern char *hardware_ip;
extern const char ports_names[MAX_PORTS_NAMES][MAX_PORT_NAME_LEN];
extern char mfr_desc[DESC_STR_LEN];
extern char hw_desc[DESC_STR_LEN];
extern char sw_desc[DESC_STR_LEN];
extern char dp_desc[DESC_STR_LEN];
extern char serial_num[SERIAL_NUM_LEN];
extern queue_conf queues[MAX_QUEUES];


void zyxel_init()
{
    zyxel_props.wildcards = 4294967292;
    zyxel_props.supported_actions_len = 2;
    zyxel_props.supported_actions = malloc(zyxel_props.supported_actions_len*sizeof(struct dev_action));
    
    zyxel_props.supported_actions[0].action_type = OFPAT_OUTPUT;
    zyxel_props.supported_actions[0].conf_fun_ptr = &output_conf_zyxel;
    zyxel_props.supported_actions[0].unconf_fun_ptr = &output_unconf_zyxel;
    
    zyxel_props.supported_actions[1].action_type = OFPAT_ENQUEUE;
    zyxel_props.supported_actions[1].conf_fun_ptr = &output_conf_zyxel;
    zyxel_props.supported_actions[1].unconf_fun_ptr = &output_unconf_zyxel;
    
    int i;
    for (i=1; i<MAX_PORTS_NAMES; i++) {
        if (strlen(ports_names[i]) == 0)
            break;
    }
    zyxel_props.ports_cnt = i;
        
    char name2[200];
    char *token, *save_ptr;
    
    // parsing IP address and port number in format "ip:port"
    strcpy(name2, hardware_ip);
    token = strtok_r(name2, ":", &save_ptr);
    if (token)
        strcpy(hardware_ip, token);
    else
        VLOG_INFO("\t port number not found in %s", name2);
    
    token = strtok_r(NULL, ":", &save_ptr);
    if (token)
        strncpy(port_number, token, 10);
    else
        VLOG_INFO("\t port number not found in %s", name2);
    
    VLOG_INFO("\t Address of zyxel device is %s", hardware_ip);
    VLOG_INFO("\t Port numer of zyxel device is %s", port_number);

    strncpy(mfr_desc,    "Zyxel", DESC_STR_LEN);
    strncpy(hw_desc,    "FSG2200HNU", DESC_STR_LEN);
    strncpy(sw_desc,    "Release X", DESC_STR_LEN);
    strncpy(dp_desc,    "Active Ethernet Multiservice IAD", DESC_STR_LEN);
    strncpy(serial_num, "", SERIAL_NUM_LEN);
    
}

static char* convert_ip_to_string(uint32_t ip_address)
{
    struct in_addr ip_addr;
    ip_addr.s_addr = htonl(ip_address);
    return inet_ntoa(ip_addr);
}

int zyxel_configure_service(uint32_t wan_address, uint32_t wan_mask, short vlan_tag, short vlan_pcp)
{
    int ret;
    char command[100];
    snprintf(command, sizeof command, "./openflow/udatapath/zyxel_client.py setwan -i %s -m %s -v %d -p %d -s %s", 
                convert_ip_to_string(wan_address), convert_ip_to_string(wan_mask), vlan_tag, vlan_pcp, hardware_ip);
    ret = system(command);
    if (ret < 0)
        VLOG_INFO("\t WAN interface creation failed");
    return ret;
}

int zyxel_unconfigure_service(uint32_t wan_address, uint32_t wan_mask, short vlan_tag, short vlan_pcp)
{
    int ret;
    char command[100];
    snprintf(command, sizeof command, "./openflow/udatapath/zyxel_client.py delwan -i %s -m %s -v %d -p %d -s %s", 
                convert_ip_to_string(wan_address), convert_ip_to_string(wan_mask), vlan_tag, vlan_pcp, hardware_ip);
    ret = system(command);
    if (ret < 0)
        VLOG_INFO("\t WAN interface deletion failed");
    return ret;
}


int output_conf_zyxel(struct inserted_flow *infl, struct dev_properties *devprop)
{
    uint16_t in_port = ntohs(infl->match.in_port);
    uint32_t wan_address = ntohl(infl->match.nw_src);
    if (wan_address == 0)
        wan_address = ntohl(infl->match.nw_dst);
    uint32_t wan_mask = 0; 
    uint16_t vlan_tag=ntohs(infl->match.dl_vlan);
    uint16_t vlan_pcp=infl->match.dl_vlan_pcp;
    uint16_t out_port=0;
    
    int i;
    for (i=0; i<=infl->auxInd; i++)
    {
        switch (ntohs(infl->actions[i].type)) 
        {
            case OFPAT_OUTPUT: 
            {
                struct ofp_action_output *oa = (struct ofp_action_output *)&infl->actions[i];
                out_port = ntohs(oa->port); 
                break;
            }

            case OFPAT_SET_VLAN_VID: 
            {
                struct ofp_action_vlan_vid *va = (struct ofp_action_vlan_vid *)&infl->actions[i];
                vlan_tag = ntohs(va->vlan_vid);
                break;
            }

            case OFPAT_SET_VLAN_PCP: 
            {
                struct ofp_action_vlan_pcp *va = (struct ofp_action_vlan_pcp *)&infl->actions[i];
                vlan_pcp = va->vlan_pcp;
                break;
            }
        }
    }

    if (infl->match.dl_type == ntohs(ETH_TYPE_ARP)) {
        VLOG_INFO("\t Skipping flowmods refereing to ARP. Skipping WAN interface creation");
        return -1;
    }
    
    //if (!(in_port == OFPP_LOCAL || out_port == OFPP_LOCAL)) {
    //    VLOG_INFO("\t Flow mod not refers to OFPP_LOCAL port. Skipping WAN interface creation");
    //    return -1;
    //}
    
    if (in_port == 0 || out_port == 0) {
        VLOG_INFO("\t Flow mod not referes to existing port. Skipping WAN interface creation");
        return -1;
    }
    
    if (vlan_tag == 0) {
        VLOG_INFO("\t Flow mod doesn't contain VLAN tag information.Skipping WAN interface creation");
        return -1;
    }
    
    struct zyxel_properties* properties = (struct zyxel_properties*) devprop;
    if (properties->service_allocator_index > MAX_SERVICES-1) {
        VLOG_INFO("\t A table for inserting services is full - no more services will be established - software needs to be restarted! ");
        return -1;
    }
    
    char wan_addr[20];
    char wan_netmask[20];
    strcpy(wan_addr, convert_ip_to_string(wan_address));
    strcpy(wan_netmask, convert_ip_to_string(wan_mask));
    
    VLOG_INFO("\t Setting WAN interface in Zyxel: match.in_port: %d, wan_address: %s, wan_mask: %s, vlan_tag: %d, pcp: %d", in_port, wan_addr, wan_netmask, vlan_tag, vlan_pcp);

    int found = 0;
    for (i = 0; i < properties->service_allocator_index; i++) {
        if (wan_address == properties->created_services[i].wan_address && vlan_tag == properties->created_services[i].vlan_tag){
            VLOG_INFO("\t Service already exists:\t wan_address=%s \t vlan_tag=%d", convert_ip_to_string(properties->created_services[i].wan_address), properties->created_services[i].vlan_tag);
            found = 1;
            break;
        }
    }

    if (found == 0) {

        if (zyxel_configure_service(wan_address, wan_mask, vlan_tag, vlan_pcp) < 0)
            return -1;

        properties->created_services[properties->service_allocator_index].in_port = in_port;
        properties->created_services[properties->service_allocator_index].flow_counter++;
        properties->created_services[properties->service_allocator_index].wan_address = wan_address;
        properties->created_services[properties->service_allocator_index].wan_mask = wan_mask;
        properties->created_services[properties->service_allocator_index].vlan_tag = vlan_tag;
        properties->created_services[properties->service_allocator_index].vlan_pcp = vlan_pcp;
        VLOG_INFO("\t Number of flows related to the service is: %d", properties->created_services[properties->service_allocator_index].flow_counter);
        properties->srv_counter++;
        properties->service_allocator_index++;
        VLOG_INFO("\t Number of services is: %d", properties->srv_counter);
    }
    else {
        properties->created_services[i].flow_counter++;
        VLOG_INFO("\t Number of flows related to the service is: %d", properties->created_services[i].flow_counter);
    }
    
    for (i = 0; i < properties->service_allocator_index; i++) {
        if (properties->created_services[i].in_port == 0)
            continue;
        VLOG_INFO("\t\t Active service is: wan_address: %s, vlan_tag: %d ", convert_ip_to_string(properties->created_services[i].wan_address), properties->created_services[i].vlan_tag);
    }
    
    return 0;
}

int output_unconf_zyxel(struct inserted_flow *infl, struct dev_properties *devprop)
{
    uint32_t wan_address = ntohl(infl->match.nw_src);
    if (wan_address == 0)
        wan_address = ntohl(infl->match.nw_dst);
    uint32_t wan_mask = 0; 
    uint16_t in_port = ntohs(infl->match.in_port);
    
    char wan_addr[20];
    char wan_netmask[20];
    strcpy(wan_addr, convert_ip_to_string(wan_address));
    strcpy(wan_netmask, convert_ip_to_string(wan_mask));
    
    VLOG_INFO("\t Deleting WAN interface in Zyxel: match.in_port: %d, wan_address: %s, wan_mask: %s", in_port, wan_addr, wan_netmask);

    if (infl->match.dl_type == ntohs(ETH_TYPE_ARP)) {
        VLOG_INFO("\t Skipping flowmods refereing to ARP. Skipping WAN interface creation");
        return -1;
    }

    struct zyxel_properties* properties = (struct zyxel_properties*) devprop;
    int i;
    int found = 0;
    for (i = 0; i < MAX_SERVICES; i++) {
        if (wan_address == properties->created_services[i].wan_address && wan_mask == properties->created_services[i].wan_mask){
            found = 1;
            if (properties->created_services[i].flow_counter > 1){
                VLOG_INFO("\t Decrementing Flow counter for the service:\t wan_address=%s \t vlan_tag=%d", convert_ip_to_string(properties->created_services[i].wan_address), properties->created_services[i].vlan_tag);
                properties->created_services[i].flow_counter--;
                VLOG_INFO("\t Number of flows related to the service is: %d", properties->created_services[i].flow_counter);
                break;
            }
            else {
                VLOG_INFO("\t Removing service:\t wan_address=%s \t vlan_tag=%d", convert_ip_to_string(properties->created_services[i].wan_address), properties->created_services[i].vlan_tag);
                
                zyxel_unconfigure_service(wan_address, wan_mask, properties->created_services[i].vlan_tag, properties->created_services[i].vlan_pcp);

                properties->created_services[i].wan_address = 0;
                properties->created_services[i].wan_mask = 0;
                properties->created_services[i].in_port = 0;
                properties->created_services[i].vlan_tag = 0;
                properties->created_services[i].flow_counter = 0;
                properties->srv_counter--;
                VLOG_INFO("\t Number of services is: %d", properties->srv_counter);
                break;
            }
        }
    }
    
    for (i = 0; i < properties->service_allocator_index; i++) {
        if (properties->created_services[i].in_port == 0)
            continue;
        VLOG_INFO("\t\t Active service is: wan_address: %s, vlan_tag: %d ", convert_ip_to_string(properties->created_services[i].wan_address), properties->created_services[i].vlan_tag);
        // TODO: defragmentation of the table - shifting non zero in_port, c_tag, s_tag items to left in table
    }
    
    if (found == 0) {
        VLOG_INFO("\t Service wan_address: %s not found ", convert_ip_to_string(wan_address));
    }
}
