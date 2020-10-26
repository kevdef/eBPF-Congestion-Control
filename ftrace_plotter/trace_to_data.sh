#!/bin/bash
awk 'FNR>11 {print $4}' trace.txt | awk -F ':' '{print $1}' > "/home/kevin/Desktop/times.txt"
awk 'FNR>11 {print $12}' trace.txt | awk -F 'snd_cwnd=' '{print $2}' > "/home/kevin/Desktop/data.txt"
