#!/bin/bash
dpctl add-flow unix:/tmp/dp0.sock in_port=6,dl_vlan=2950,dl_src=52:54:00:a0:21:1f,dl_type=0x806,nw_dst=172.16.0.11,actions=mod_vlan_vid:2950,output:-4
sleep 2

dpctl add-flow unix:/tmp/dp0.sock in_port=6,dl_vlan=2950,dl_src=52:54:00:a0:21:1f,dl_type=0x800,nw_dst=172.16.0.11,actions=mod_vlan_vid:2950,output:3


sleep 60



dpctl del-flows unix:/tmp/dp0.sock in_port=6,dl_vlan=2950,dl_src=52:54:00:a0:21:1f,dl_type=0x800,nw_dst=172.16.0.11
sleep 2

dpctl del-flows unix:/tmp/dp0.sock in_port=6,dl_vlan=2950,dl_src=52:54:00:a0:21:1f,dl_type=0x806,nw_dst=172.16.0.11
