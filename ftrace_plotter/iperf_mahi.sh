#!/bin/bash 
#mm-delay 60 
mm-delay 50 mm-link mm-uplink.txt mm-downlink.txt -- iperf3 -c 10.0.2.15 -p 5002 -t 100 -T "client"
#'iperf3 -c 10.0.2.15 -p 5002 -w 1K'
#iperf3 -c 100.64.0.4 -w 1K
