 /*
 * Copyright (c) 2013 PSNC
 * Autors: Szymon Kuc, Damian Parniewicz
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <curl/curl.h>
#include "intune.h"

#include "vlog.h"
#define THIS_MODULE VLM_datapath

struct intune_properties intune_props;
static bool true_initialization = true;
char port_number[10];

extern char *hardware_ip;
extern const char ports_names[MAX_PORTS_NAMES][MAX_PORT_NAME_LEN];
extern char mfr_desc[DESC_STR_LEN];
extern char hw_desc[DESC_STR_LEN];
extern char sw_desc[DESC_STR_LEN];
extern char dp_desc[DESC_STR_LEN];
extern char serial_num[SERIAL_NUM_LEN];
extern queue_conf queues[MAX_QUEUES];


// CODE could not be published
