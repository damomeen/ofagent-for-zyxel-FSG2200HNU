/*
 * Distributed under the terms of the GNU GPL version 2.
 * Copyright (c) 2013 PSNC
 */

#ifndef OFAGENT_H
#define OFAGENT_H 1


#include <stdlib.h>
#include "packets.h"
#include "openflow/openflow.h"

struct inserted_flow {
    struct ofp_match match;
    struct ofp_action_header *actions;
    size_t actions_len;
    int actions_num;
    /*Saved device configurations corresponding to the actions of the flow*/
    void **dev_configs;
    /*TODO: Describe this*/
    int auxInd;
};

typedef struct _queue_conf {
    int queue_id;
    int port_id;     //  attached port to queue
    int min_rate;   // percentange of physicial bandwidth, expresed in promiles (1000 disabled)
    int max_rate;  // percentange of physicial bandwidth, expresed in promiles (1000 disabled)
    bool enabled; 
} queue_conf;

#define MAX_QUEUES 100
queue_conf queues[MAX_QUEUES];

typedef struct {
    char *data;
    size_t size;
    int bytesCopied;
    int bytesRemaining;
} MemoryStruct;

#define MAX_PORTS_NAMES 50
#define MAX_PORT_NAME_LEN 200
const char ports_names[MAX_PORTS_NAMES][MAX_PORT_NAME_LEN];

/*
Structure that associates the action type with the function to configure the device
when the action is applied, and with the function to unconfigure the device when the
same action is unapplied (after the flow deletion)
*/
struct dev_action {
    uint16_t action_type;
    int (*conf_fun_ptr)(struct inserted_flow*, struct dev_properties*);
    int (*unconf_fun_ptr)(struct inserted_flow*, struct dev_properties*);
};

struct dev_properties {
    uint32_t wildcards; //specify the flow mod fields on which the device should react
    struct dev_action* supported_actions; //action types supported by the device
    int supported_actions_len;
};

void init_port_list();
void add_port(char* port_name, int index);
void read_queue_config();
void apply_actions(struct inserted_flow *infl, struct dev_properties *devprop);
void unapply_actions(struct inserted_flow *infl, struct dev_properties *devprop);
int my_flow_fields_match(struct ofp_match *a, struct ofp_match *b);
void handleFlowMod(struct ofp_flow_mod *ofm);

#endif /* OFAGENT_H */