#!/bin/bash
dpctl add-flow unix:/tmp/zyxel_2.sock in_port=0xfffe,dl_type=0x800,nw_src=172.16.0.11,actions=mod_vlan_vid:3954,mod_vlan_pcp:4,output:2
sleep 30
dpctl del-flows unix:/tmp/zyxel_2.sock in_port=0xfffe,dl_type=0x800,nw_src=172.16.0.11

