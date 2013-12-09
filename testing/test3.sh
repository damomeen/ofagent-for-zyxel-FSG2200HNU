#!/bin/bash
dpctl add-flow unix:/var/run/dp0.sock in_port=2,dl_vlan=3,nw_dst=192.168.1.1,actions=output:1
sleep 2
dpctl del-flows unix:/var/run/dp0.sock in_port=2,dl_vlan=3,nw_dst=192.168.1.1
sleep 2
