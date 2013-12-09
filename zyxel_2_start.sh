#!/bin/bash
#killall ofprotocol
#killall ofdatapath
#sleep 2

# Arguments
# d - datapath id
# n - name of hardware platform (e.g.: intune, milegate)
# a - address of the management interface of a network device
# u - username for accessing management interface of a network device
# p - password for accessing maangement interface of a network device
# i - list of data plane interface names within a network device (there shouldn't be spaces on the list)
#       each interface has the following format: {device-name}/{device-MAC}:shelf/{id}/slot/{id}/port/{id}:{pcp-to-tc}
#        {pcp-to-tc} mapping is composed of 8 digits, first digit is tc mapping for pcp=0, second digit is tc mapping for pcp=1, etc.
#        Example: iNX8000/284C5300001:shelf/1/slot/7/port/2:0003050

openflow/udatapath/ofdatapath punix:/tmp/zyxel_2.sock -d 5067f06024e4 -n zyxel -a 10.0.3.2:80  -i eth1,eth2 --no-local-port --log-file=./of_agent_zyxel.log &
sleep 2

# Arguments
# tcp - address of OpenFlow controller

ofprotocol unix:/tmp/zyxel_2.sock tcp:10.0.1.100 &

