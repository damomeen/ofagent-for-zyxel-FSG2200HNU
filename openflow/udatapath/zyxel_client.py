#!/usr/bin/env python

"""
 * Distributed under the terms of the GNU GPL version 2.
 * Copyright (c) 2013 PSNC
 * Autor: damian.parniewicz at man.poznan.pl
 """

import socket, time
import uuid, random
import thread
import re
import sys
import httplib
import copy
import logging
from urllib import urlencode
from optparse import OptionParser


# Read: http://www.yolinux.com/TUTORIALS/ForkExecProcesses.html

MODULE_NAME = 'zyxel_client'
__version__ = '0.1'

SERVER = None
WAN_INTERFACES = range(1,9)

#====================================================================== 
HEADERS = {"Cookie": "session=%s"  % str(uuid.uuid4()).replace("-", ""), 
                  "Connection": "keep-alive",
                  "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
                  "Accept-Language": "pl,en-us;q=0.7,en;q=0.3",
                  "Accept-Encoding": "gzip, deflate"}

LOGIN_BODY = """

-----------------------------27803126511535
Content-Disposition: form-data; name="UserName"

admin
-----------------------------27803126511535
Content-Disposition: form-data; name="loginPassword"

ZyXEL ZyWALL Series
-----------------------------27803126511535
Content-Disposition: form-data; name="hiddenPassword"

addonas
-----------------------------27803126511535
Content-Disposition: form-data; name="Submit"

Login
-----------------------------27803126511535
Content-Disposition: form-data; name="submitValue"

1
-----------------------------27803126511535--
""".replace("\n", "\r\n")

#======================================================================
# CONFIGURATION FORMS

WAN_CONF = {
    'wanIntfType': 'IPoE',
    'serviceName': 'Addonas-OpenFlow',
    'usedVlanID': '',
    'vlanId': '200',
    'vlanPri': '0',
   'serviceType0': 'Data',
    'serviceTypeValue': 'Data',
    'queueIdx': '4',
    'queueIdxValue': '4',
    'usedVoIPServiceType': '0',
    'usedMgmtServiceType': '1',
    'wanIndex': '',
}

WAN_GATEWAY = {
    'interface': '',
    'ntwkPrtcl': '',
    'serviceName': '',
    'vlanMuxPr': '',
    'vlanMuxId': '',
    'lyr2IntrfcLink': '',
    'pppIdx': '',
    'IpIdx': '',
    'editFlags': '',
    'ConnSelect': '',
    'serviceTypeValue': '',
    'queueIdxValue': '',
    'wanIpType': 'Static',
    'wanIpAddress': '172.16.100.2',
    'wanSubnetMask': '255.255.255.0',
    'wanIntfGateway': '172.16.100.1', 
    'enableNAT': 'enableNAT',
    'enableIcmp': 'enableIcmp',
    'wanSrvConfigBackflag': '0',
}

WAN_SUM2 = {
    'wanDns': 'static',
    'dnsPrimary': '8.8.8.8',
    'dnsSecondary': '0.0.0.0',
    'ifcBackflag': '0',
    'wanIntfType': 'IPoE',
    'wanIpType': 'Static',
}

WAN_SAVE = {
    'isSubmit': 'save',
    'ntwkBackflag': '0',
}

WAN_DELETE = {
    'isSubmit': 'del',
    'delidx': '160i1',
    'defaultGW': 'wan1',
}

#======================================================================
# UTILS

def date():
    return time.strftime("%a, %d %b %Y %H:%M:%S GMT", time.gmtime())
    
def now():
    return time.strftime("%H:%M:%S", time.gmtime())
    
def parse_vlan(req):
    PATTERN = """<td>(\d+)</td>"""
    match = re.search(PATTERN, req)
    if match:
        return match.group(1)

def generate_gateway(ip_address):
    # gateway IP is ended with '.254'
    ip = ip_address.split('.')
    if len(ip) == 4:
        ip[3] = '254'
    else:
        logger.error('Wrong IP address %s', ip_address)
    return '.'.join(ip)
    
def assign_wan(services):
    used_wan_ids = services.values()
    for wan_id in WAN_INTERFACES:
        if wan_id not in used_wan_ids:
            return wan_id
    return None # no WAN ID was assigned

#======================================================================

LAST_URI = "/login.cgi"
def genericRequest(method, uri, body="", form={}, contentType=None):
    # prepare HTTP headers
    headers = copy.deepcopy(HEADERS)
    if contentType != None:
        headers["Content-Type"] = contentType
        
    global LAST_URI
    headers["Referer"] = "http:/%s" % LAST_URI
    LAST_URI = uri
    
    if form != {}:
        body = urlencode(form)
        headers["Content-Type"] = "application/x-www-form-urlencoded"
    
    # send HTTP request
    try:
        conn = httplib.HTTPConnection(SERVER, timeout=10)
        conn.request(method, uri, body, headers)
        response = conn.getresponse()
        logger.info("HTTP %s %s request. Response is: %s, %s", method, uri, response.status, response.reason)
        return response
    except:
        logger.error("Problem with communication to Zyxel device", exc_info=True)
        sys.exit(-1)

#======================================================================

def loginReq1():
    return genericRequest(method='POST', uri='/login.cgi')
    
def loginReq2():
    return genericRequest(method='POST', uri='/login.cgi', body=LOGIN_BODY, contentType="multipart/form-data; boundary=---------------------------27803126511535")
    
def logout():
    HEADERS["Cookie"] = "session="
    return genericRequest(method='GET', uri='/login.cgi')

def get_all_WAN():
    services = {}
    response = genericRequest(method='GET', uri='/wancfg.cgi')
    data = response.read()
    for i in WAN_INTERFACES:
        found = False
        found_cnt = 0
        for line in data.splitlines():
            index = line.find('wan%d' % i)
            if found == False and index != -1:
                found = True
            elif found == True and found_cnt < 3:
                found_cnt += 1
            elif  found == True and found_cnt >= 3:
                vlan = parse_vlan(line)
                services[vlan] = i
                break
    return services
                
    
    
def get_WAN(wan_if):
    return genericRequest(method='GET', uri='/wansrv.cgi?1%s0i1' % str(wan_if))
    
def confWAN_1():
    return genericRequest(method='POST', uri='/ipoe.cgi', form=WAN_CONF)
    
def confWAN_2():
    return genericRequest(method='POST', uri='/ifcgateway.cgi', form=WAN_GATEWAY)
    
def confWAN_3():
    return genericRequest(method='POST', uri='/ntwksum2.cgi', form=WAN_SUM2)
    
def confWAN_save():
    return genericRequest(method='POST', uri='/wancfg.cgi', form=WAN_SAVE)
    
def del_wan():
    return genericRequest(method='POST', uri='/wancfg.cgi', form=WAN_DELETE)    

#======================================================================

def set_wan_interface(options):
    WAN_CONF['vlanId'] = options.vlan
    WAN_CONF['vlanPri'] = options.pcp
    WAN_GATEWAY['wanIpAddress'] = options.wanIP
    WAN_GATEWAY['wanSubnetMask'] = "255.255.255.0" #options.wanNetMask
    WAN_GATEWAY['wanIntfGateway'] = generate_gateway(options.wanIP)
    WAN_GATEWAY['enableNAT'] = 'enableNAT'
    
    loginReq1()
    loginReq2()
    services = get_all_WAN()
    logger.info("Services before %s", services)
    if options.vlan in services:
        logger.warn("VLAN tag %s already configured for wan%s", options.vlan, services.get(options.vlan))
    wan_id = assign_wan(services)
    if wan_id is None:
        logger.error("All WAN interfaces are used - cannot create a service")
        sys.exit(-1)
    else:
        logger.error("Assigning wan%s for a service", wan_id)
        
    get_WAN(wan_id)
    confWAN_1()      
    confWAN_2()      
    confWAN_3()      
    confWAN_save()  
    #services = get_all_WAN()
    #logger.info("Services after %s", services)
    logout()
    
def del_wan_interface(options):
    loginReq1()
    loginReq2()
    services = get_all_WAN()
    logger.info("Services before %s", services)
    wan_id = services.get(options.vlan)
    if wan_id:
        logger.info("Deleting wan%s", wan_id)
        WAN_DELETE['delidx'] = '1%s0i1' % str(wan_id)
        del_wan()
    #services = get_all_WAN()
    #logger.info("Services after %s", services)
    logout()
    
def login_logout(options):
    loginReq1()
    loginReq2()
    response = logout()
    print response.getheaders()
    
def getServices(options):
    loginReq1()
    loginReq2()
    services = get_all_WAN()
    logger.info("Services are %s", services)
    for vlan, wanId in services.items():
        print "wan%s - vlan %s" % (wanId, vlan)
    logout()
    
#======================================================================

if __name__ == "__main__":
        # optional command-line arguments processing
    usage="usage: %prog setwan|delwan [options]"
    parser = OptionParser(usage=usage, version="%prog " + __version__)
    parser.add_option("-i", "--wan_ip", dest="wanIP", default='192.168.1.1', help="IP address of WAN interface")
    parser.add_option("-m", "--wan_netmask", dest="wanNetMask", default='255.255.255.0', help="netmask of WAN interface address")
    parser.add_option("-v", "--vlan", dest="vlan", default='200', help="VLAN used over WAN interface")
    parser.add_option("-p", "--pcp", dest="pcp", default='0',    help="PCP used over WAN interface")
    parser.add_option("-s", "--server", dest="server", default='10.0.3.2',  help="IP address of the Zyxel device")
    options, args = parser.parse_args()   

    # creation of logging infrastructure
    logging.basicConfig(filename = "./%s.log" % MODULE_NAME,
                        level    = logging.DEBUG,
                        format   = "%(levelname)s - %(asctime)s - %(name)s - %(message)s")
    logger = logging.getLogger(MODULE_NAME)
    
    SERVER = options.server
    
    # mandatory command-line arguments processing
    if len(args) == 0:
        print usage
        sys.exit(-1)
    if 'setwan' == args[0]:
        logger.info('Setting WAN interface %s', options)
        set_wan_interface(options)
    elif 'delwan' == args[0]:
        logger.info('Deleting WAN interface %s', options)
        del_wan_interface(options)
    elif 'login_logout' == args[0]:
        logger.info('Testing loging and logout')
        login_logout(options)
    elif 'getservices' == args[0]:
        logger.info('Getting services')
        getServices(options)
    else:
        print "Unknown command"
        print usage
        sys.exit(-1)
    logger.info("Finished.")
    sys.exit(0)