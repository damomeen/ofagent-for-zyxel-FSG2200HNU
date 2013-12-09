 /*
 * Copyright (c) 2013 PSNC
 * Autors: Szymon Kuc, Damian Parniewicz
 */
 
#ifndef INTUNE_H
#define INTUNE_H 1

#include "ofagent.h"
#include <stdint.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>

#define MAX_EVCS_SERVICES 1000
#define MAX_INTUNE_PORTS 10
#define MAX_INTUNE_TRAFFIC_CLASSES 8

#define PORT_BANDWIDTH 10000000000

typedef struct {
    bool active;
	int priority;
	int64_t ingressCIR;	
	int64_t ingressCBS;
	int64_t ingressEIR;
	int64_t ingressEBS;
	bool ingressColorMode;
	int ingressCouplingFlag;

} eLineTrafficMappingTable;

typedef struct connectionTerminationPoint {
	char* parentTp[MAX_EVCS_SERVICES];
	unsigned short ivid;
	char* portTpRoleState;
	eLineTrafficMappingTable bandwidthProfile;

} connectionTp;

struct intune_port_url {
    char *cmeGatewayIPAddress;
    char meName[200];
    char tpPath[200];
    short portId;
};

struct evc_params {
    uint16_t vid;
    uint16_t in_port;
    uint16_t out_port;
    short flow_counter;
    char *evcID;
};


struct intune_properties {
    uint32_t wildcards;
    struct dev_action* supported_actions;
    int supported_actions_len;
    struct intune_port_url of_to_intune_portmap[MAX_INTUNE_PORTS];
    short int pcpToTcMappingTable[MAX_INTUNE_PORTS][MAX_INTUNE_TRAFFIC_CLASSES];
    struct evc_params created_evcs[MAX_EVCS_SERVICES];
    short evc_counter;
    short service_allocator_index;
    char *cmeGatewayIPAddress;
};

struct intune_conf {
    char* evcID;
};

extern struct intune_properties intune_props;

#endif /* INTUNE_H */