#!/bin/bash
dpctl add-flow unix:/tmp/zyxel_2.sock in_port=1,dl_vlan=3954,dl_src=52:54:00:a0:21:1f,dl_type=0x800,nw_dst=172.16.0.11,actions=mod_vlan_vid:3954,output:3
sleep 30
dpctl del-flows unix:/tmp/zyxel_2.sock in_port=1,dl_vlan=3954,dl_src=52:54:00:a0:21:1f,dl_type=0x800,nw_dst=172.16.0.11

