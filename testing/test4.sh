#!/bin/bash
dpctl add-flow unix:/var/run/dp0.sock in_port=1,dl_vlan=100,dl_src=00:0A:E4:25:6B:B0,actions=output:2
sleep 2
dpctl add-flow unix:/var/run/dp0.sock in_port=1,dl_vlan=100,nw_dst=192.168.1.1,actions=output:2
sleep 2
dpctl add-flow unix:/var/run/dp0.sock in_port=1,dl_vlan=2,nw_dst=192.168.1.1,actions=output:2
sleep 2
dpctl add-flow unix:/var/run/dp0.sock in_port=1,dl_vlan=100,tp_dst=80,actions=output:2
sleep 2
dpctl add-flow unix:/var/run/dp0.sock in_port=2,dl_vlan=100,dl_src=00:0A:E4:25:6B:B0,actions=output:1
sleep 2
dpctl add-flow unix:/var/run/dp0.sock in_port=2,dl_vlan=2,dl_src=00:0A:E4:25:6B:B0,actions=output:1
sleep 2
dpctl add-flow unix:/var/run/dp0.sock in_port=2,dl_vlan=100,nw_dst=192.168.1.1,actions=output:1
sleep 2
dpctl add-flow unix:/var/run/dp0.sock in_port=2,dl_vlan=3,nw_dst=192.168.1.1,actions=output:1
sleep 2
dpctl add-flow unix:/var/run/dp0.sock in_port=2,dl_vlan=100,tp_dst=80,actions=output:1
sleep 2
dpctl del-flows unix:/var/run/dp0.sock in_port=1,dl_vlan=100,dl_src=00:0A:E4:25:6B:B0
sleep 2
dpctl del-flows unix:/var/run/dp0.sock in_port=1,dl_vlan=100,nw_dst=192.168.1.1
sleep 2
dpctl del-flows unix:/var/run/dp0.sock in_port=1,dl_vlan=100,tp_dst=80
sleep 2
dpctl del-flows unix:/var/run/dp0.sock in_port=2,dl_vlan=100,dl_src=00:0A:E4:25:6B:B0
sleep 2
dpctl del-flows unix:/var/run/dp0.sock in_port=1,dl_vlan=2,nw_dst=192.168.1.1
sleep 2
dpctl del-flows unix:/var/run/dp0.sock in_port=2,dl_vlan=100,nw_dst=192.168.1.1
sleep 2
dpctl del-flows unix:/var/run/dp0.sock in_port=2,dl_vlan=100,tp_dst=80
sleep 2
dpctl del-flows unix:/var/run/dp0.sock in_port=2,dl_vlan=3,nw_dst=192.168.1.1
sleep 2
dpctl del-flows unix:/var/run/dp0.sock in_port=2,dl_vlan=2,dl_src=00:0A:E4:25:6B:B0
sleep 2

