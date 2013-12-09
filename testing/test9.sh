#!/bin/bash
dpctl add-flow unix:/tmp/dp0.sock in_port=1,dl_vlan=3954,dl_src=52:54:00:a0:21:1f,dl_type=0x800,nw_dst=172.16.0.11,dl_vlan_pcp=3,actions=mod_vlan_vid:3954,enqueue:3:1
sleep 10
dpctl del-flows unix:/tmp/dp0.sock in_port=1,dl_vlan=3954,dl_src=52:54:00:a0:21:1f,dl_type=0x800,nw_dst=172.16.0.11,dl_vlan_pcp=3

