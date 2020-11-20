#!/bin/bash
sysctl -w net.ipv4.ip_forward=1
chmod a+r /sys/kernel/debug/tracing/ 
echo > /sys/kernel/debug/tracing/trace
echo 1 > /sys/kernel/debug/tracing/events/tcp/tcp_probe/enable
sudo -u kevin ./iperf_mahi_s.sh  & sudo -u kevin ./iperf_mahi.sh
sudo killall iperf3
echo 0 > /sys/kernel/debug/tracing/events/tcp/tcp_probe/enable
su kevin -c "touch ~/Desktop/trace.txt"
cat /sys/kernel/debug/tracing/trace > "/home/kevin/Desktop/trace.txt"
