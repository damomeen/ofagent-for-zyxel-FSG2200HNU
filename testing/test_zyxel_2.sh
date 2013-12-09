#!/bin/bash
dpctl add-flow unix:/tmp/zyxel_2.sock in_port=0xfffe,dl_type=0x806,nw_src=172.16.0.11,actions=mod_vlan_vid:2950,mod_vlan_pcp:4,output:2
sleep 2

dpctl add-flow unix:/tmp/zyxel_2.sock in_port=0xfffe,dl_type=0x800,nw_src=172.16.0.11,actions=mod_vlan_vid:2950,mod_vlan_pcp:4,output:2

sleep 2
dpctl add-flow unix:/tmp/zyxel_2.sock in_port=2,dl_vlan=2950,dl_vlan_pcp=4,dl_type=0x806,nw_dst=172.16.0.11,actions=strip_vlan,output:0xfffe

sleep 2
dpctl add-flow unix:/tmp/zyxel_2.sock in_port=2,dl_vlan=2950,dl_vlan_pcp=4,dl_type=0x800,nw_dst=172.16.0.11,actions=strip_vlan,output:0xfffe

sleep 60



dpctl del-flows unix:/tmp/zyxel_2.sock in_port=0xffffe,dl_type=0x800,nw_src=172.16.0.11
sleep 2

dpctl del-flows unix:/tmp/zyxel_2.sock in_port=0xffffe,dl_type=0x806,nw_src=172.16.0.11

sleep 2
dpctl del-flows unix:/tmp/zyxel_2.sock in_port=2,dl_vlan=2950,dl_vlan_pcp=4,dl_type=0x806,nw_dst=172.16.0.11

sleep 2
dpctl del-flows unix:/tmp/zyxel_2.sock in_port=2,dl_vlan=2950,dl_vlan_pcp=4,dl_type=0x800,nw_dst=172.16.0.11
