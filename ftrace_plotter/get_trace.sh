#!/bin/bash
chmod a+r /sys/kernel/debug/tracing/
echo > /sys/kernel/debug/tracing/trace
echo 1 > /sys/kernel/debug/tracing/events/tcp/tcp_probe/enable
iperf3 -s &
iperf3 -c localhost -i 1 -t 10
echo 0 > /sys/kernel/debug/tracing/events/tcp/tcp_probe/enable
su kevin -c "touch ~/Desktop/trace.txt"
cat /sys/kernel/debug/tracing/trace > "/home/kevin/Desktop/trace.txt"
