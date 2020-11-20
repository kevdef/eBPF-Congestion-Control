#!/bin/bash
#($6="src=[::ffff:100.64.0.1]:5201")
#if ($6 ~ /src=100/)
awk 'FNR>11 {if ($6 ~ /src=100/) print $6, $4, $12, $17}' trace.txt > "/home/kevin/Desktop/trace_filtered.txt"
awk '{print $2}' trace_filtered.txt | awk -F ':' '{print $1}' > "/home/kevin/Desktop/times.txt"
awk '{print $3}' trace_filtered.txt | awk -F 'snd_cwnd=' '{print $2}' > "/home/kevin/Desktop/data.txt"
