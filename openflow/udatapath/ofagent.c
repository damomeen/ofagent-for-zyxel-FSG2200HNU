/*
 * Distributed under the terms of the GNU GPL version 2.
 * Copyright (c) 2013 PSNC
 */
 
#include "intune.h"
#include "milegate.h"
#include "zyxel.h"
#include "ofp-print.h"

#include "vlog.h"

#define THIS_MODULE VLM_datapath

#define QUEUE_CONF_FILE "queues.conf"

#define MAX_NUMBER_OF_FLOWS 10000

struct inserted_flow inserted_flows[MAX_NUMBER_OF_FLOWS];

extern char *hardware_name;

void init_port_list() 
{
    int i;
    char* tmp = "";
    for(i=0; i<MAX_PORTS_NAMES; i++){
        strcpy(&ports_names[i], tmp);
    }
}

void add_port(char* port_name, int index)
{
    if (index < MAX_PORTS_NAMES) {
        strcpy(&ports_names[index], port_name);
    }
}

void read_queue_config()
{
    FILE *fp; 
    if ((fp=fopen(QUEUE_CONF_FILE, "r"))==NULL) {
        VLOG_INFO ("\t Cannot open queue configuration file: %s", QUEUE_CONF_FILE);
        return;
    }
    
    char buffer[10240];
    int i,j;
    for (i = 0; i < MAX_QUEUES && fgets(buffer, 10240, fp); i++)
    {
        int pos;
        pos = sscanf(buffer, "%d,%d,%d,%d",
                   &queues[i].queue_id,
                   &queues[i].port_id,
                   &queues[i].min_rate,
                   &queues[i].max_rate);
        queues[i].enabled = true;
        if (pos != 4) {
            fprintf(stderr, "Invalid format: <%s>\n", buffer);
            fclose (fp);
            return;
        }
    }
    // these queue configs was not read from file
    for (;i < MAX_QUEUES; i++){
        queues[i].enabled = false;
    }
            
    fclose (fp);
}

int my_flow_fields_match(struct ofp_match *a, struct ofp_match *b)
{
    return    ((a->in_port == b->in_port)
            && (a->dl_vlan == b->dl_vlan)
            && (a->dl_vlan_pcp == b->dl_vlan_pcp)
            && (eth_addr_equals(a->dl_src, b->dl_src))
            && (eth_addr_equals(a->dl_dst, b->dl_dst))
            && (a->dl_type == b->dl_type)
            && (a->nw_tos == b->nw_tos)
            && (a->nw_proto == b->nw_proto)
            && (a->nw_src == b->nw_src)
            && (a->nw_dst == b->nw_dst)
            && (a->tp_src == b->tp_src)
            && (a->tp_dst == b->tp_dst));
}


void print_action(struct ofp_action_header *ah) 
{
    size_t len =  ntohs(ah->len);
    
    switch (ntohs(ah->type)) {
    case OFPAT_OUTPUT: {
        struct ofp_action_output *oa = (struct ofp_action_output *)ah;
        uint16_t port = ntohs(oa->port); 
        VLOG_INFO("\t action: output:%d", port);
        break;
    }

    case OFPAT_ENQUEUE: {
        struct ofp_action_enqueue *ea = (struct ofp_action_enqueue *)ah;
        uint16_t port = ntohs(ea->port);
        uint32_t queue = ntohl(ea->queue_id);
        VLOG_INFO("\t action: enqueue:%d, %d", port, queue);
        break;
    }

    case OFPAT_SET_VLAN_VID: {
        struct ofp_action_vlan_vid *va = (struct ofp_action_vlan_vid *)ah;
        VLOG_INFO("\t action: mod_vlan_vid:%d ", ntohs(va->vlan_vid));
        break;
    }

    case OFPAT_SET_VLAN_PCP: {
        struct ofp_action_vlan_pcp *va = (struct ofp_action_vlan_pcp *)ah;
        VLOG_INFO("\t action: mod_vlan_pcp:%d ", va->vlan_pcp);
        break;
    }

    case OFPAT_STRIP_VLAN:
        VLOG_INFO("\t action: strip_vlan \n");
        break;

    case OFPAT_SET_DL_SRC: {
        struct ofp_action_dl_addr *da = (struct ofp_action_dl_addr *)ah;
        VLOG_INFO("\t action: mod_dl_src %d ", ETH_ADDR_ARGS(da->dl_addr));
        break;
    }

    case OFPAT_SET_DL_DST: {
        struct ofp_action_dl_addr *da = (struct ofp_action_dl_addr *)ah;
        VLOG_INFO("\t action: mod_dl_dst %d ", ETH_ADDR_ARGS(da->dl_addr));
        break;
    }

    case OFPAT_SET_NW_SRC: {
        struct ofp_action_nw_addr *na = (struct ofp_action_nw_addr *)ah;
        VLOG_INFO("\t action: mod_nw_src %d", IP_ARGS(&na->nw_addr));
        break;
    }

    case OFPAT_SET_NW_DST: {
        struct ofp_action_nw_addr *na = (struct ofp_action_nw_addr *)ah;
        VLOG_INFO("\t action: mod_nw_dst %d", IP_ARGS(&na->nw_addr));
        break;
    }

    case OFPAT_SET_NW_TOS: {
        struct ofp_action_nw_tos *nt = (struct ofp_action_nw_tos *)ah;
        VLOG_INFO("\t action: mod_nw_tos %d", nt->nw_tos);
        break;
    }

    case OFPAT_SET_TP_SRC: {
        struct ofp_action_tp_port *ta = (struct ofp_action_tp_port *)ah;
        VLOG_INFO("\t action: mod_tp_src %d", ntohs(ta->tp_port));
        break;
    }

    case OFPAT_SET_TP_DST: {
        struct ofp_action_tp_port *ta = (struct ofp_action_tp_port *)ah;
        VLOG_INFO("\t action: mod_tp_dst %d", ntohs(ta->tp_port));
        break;
    }

    case OFPAT_VENDOR: {
        struct ofp_action_vendor_header *avh 
                = (struct ofp_action_vendor_header *)ah;
        if (len < sizeof *avh) {
            VLOG_INFO("\t action: ***ofpat_vendor truncated***");
            return -1;
        }
        /* Identify individual vendor actions */
        if (1 == 0) {
            /* Code to output vendor action */
        } else {
            VLOG_INFO("vendor action:0x%x", ntohl(avh->vendor));
        }
        break;
    }

    default:
        VLOG_INFO("decoder %d not implemented", ntohs(ah->type));
        break;
    }
}

void apply_actions(struct inserted_flow *infl, struct dev_properties *devprop)
{
    int i;
    uint8_t *p = (uint8_t *)infl->actions;
    size_t actions_len = infl->actions_len;
    int success = 0;
    /*For each action type of the actions included in the inserted flow,
    find the corresponding type in the actions supported by the device and execute the action*/
    while (actions_len > 0) {
        struct ofp_action_header *ah = (struct ofp_action_header *)p;
        size_t len = htons(ah->len);
        print_action(ah);
        for (i = 0; i < devprop->supported_actions_len; i++)
            if (ah->type == htons(devprop->supported_actions[i].action_type)) {
                struct dev_action* sa = &devprop->supported_actions[i];
                /*Configure the device using the function stored in the function pointer*/
                success = sa->conf_fun_ptr(infl, devprop);   // if sucess is -1 then generate notification about flowmod failure
                break;
            }
        /*Increase p to point to another action*/
        p += len;
        actions_len -= len;
        /* Index of the next action to be performed*/
        infl->auxInd++;
    }
}

void unapply_actions(struct inserted_flow *infl, struct dev_properties *devprop)
{
    int i;
    infl->auxInd = 0;
    uint8_t *p = (uint8_t *)infl->actions;
    size_t actions_len = infl->actions_len;
    while (actions_len > 0) {
        struct ofp_action_header *ah = (struct ofp_action_header *)p;
        size_t len = htons(ah->len);
        for (i = 0; i < devprop->supported_actions_len; i++)
            if (ah->type == htons(devprop->supported_actions[i].action_type)) {
                struct dev_action* sa = &devprop->supported_actions[i];
                if (sa->unconf_fun_ptr)
                    sa->unconf_fun_ptr(infl, devprop);
                else
                    VLOG_INFO("\t Unapply action not handled %d", sa->action_type);
                break;
            }

        p += len;
        actions_len -= len;
        infl->auxInd++;
    }
}

void handleFlowMod(struct ofp_flow_mod *ofm)
{  
    struct dev_properties *dev_prop;
    
    read_queue_config();    
    
    if(strcmp(hardware_name, "milegate") == 0) {
        milegate_init();
        dev_prop = (struct dev_properties*) &milegate_props;
    }
    else if(strcmp(hardware_name, "intune") == 0) {
        intune_init();
        dev_prop = (struct dev_properties*) &intune_props;
    }
    else 
    {
        if(strcmp(hardware_name, "zyxel") == 0) {
           zyxel_init();
           dev_prop = (struct dev_properties*) &zyxel_props;
        }
        else {
           VLOG_INFO("\t Platform name %s not recongized !", hardware_name);
           return;
        }
    }
        
    static int flow_counter = 0;
    if (flow_counter > MAX_NUMBER_OF_FLOWS-1) {
        VLOG_INFO("\t A table for inserting flows is full - no more flows will be handled - software needs to be restarted!");
        return;
    }
    int i;

    /* Check if wildcards in flow_mod match those in device properties*/
    if (~ofm->match.wildcards & ~dev_prop->wildcards != ~dev_prop->wildcards) return;

    VLOG_INFO("\t Wildcards: 0x%x", ofm->match.wildcards);
    char* match_str = ofp_match_to_string(&ofm->match, 5);
    VLOG_INFO("\t %s", match_str);
    free(match_str);
    
    uint16_t command = ntohs(ofm->command);
    /*Command which adds a flow to a flow table*/
    if (command == OFPFC_ADD){
        VLOG_INFO("\t Executing OFPFC_ADD command");
        /*Copy the command's match and actions value into a global table of inserted flows*/
        struct inserted_flow *infl = &inserted_flows[flow_counter];
        memcpy(&infl->match, &ofm->match, sizeof(ofm->match));
        infl->actions_len = ntohs(ofm->header.length) - sizeof *ofm;
        infl->actions = malloc(infl->actions_len);
        memcpy(infl->actions, ofm->actions, infl->actions_len);
        /*Each action has a corresponding device configuration state that is an effect of applied action*/
        /*Allocate the memory for pointers to device configurations (now these are of type void*),
        number of pointers to allocate memory for equals to the number of actions in the flow mod message */
        infl->actions_num = (ntohs(ofm->header.length) - sizeof *ofm) / sizeof(struct ofp_action_header);
        infl->dev_configs = malloc(infl->actions_num * sizeof(void*));
        /*This pointer description*/
        infl->auxInd = 0;
        /*Function that applies the actions from the inserted flow to the device*/
        apply_actions(infl, dev_prop);
    
        /* Variable that counts the number of inserted flows*/
        flow_counter++;
    }
    /*Command which deletes a flow from a flow table*/
    else if (command == OFPFC_DELETE){
        VLOG_INFO("\t Executing OFPFC_DELETE command");
        /*In the inserted flows table find the flow whose fields match the fields of the flow to be deleted*/
        for (i = 0; i < flow_counter; i++){
            struct inserted_flow *infl = &inserted_flows[i];
            if (my_flow_fields_match(&infl->match, &ofm->match)){ //my function for checking the match
                /*Unapply actions associated with the flow to be deleted*/
                unapply_actions(infl, dev_prop);
                /*Delete the match value to prevent it from being compared in another flow deletion*/
                memset(&infl->match, 0, sizeof(infl->match));
                free(infl->actions);
                free(infl->dev_configs);
            }
        }
    }
    else  {
        VLOG_INFO("\t No handler for FLOWMOD command: %d", command);
    }
    return;
}
