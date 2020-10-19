import matplotlib.pyplot as plt
import numpy as np
import os

def get_cwnd(line):
    words = line.split()
    cwnd = ""
    for word in words:
        if "snd_cwnd" in word:
            cwnd = word[9:]
    return cwnd

def extract_cwnd(): 
    traceEntries= [] 
    count = 0
    with open ('trace.txt', 'rt') as myfile:
        for line in myfile:
            count+=1
            if(count%692==0):
                traceEntries.append(get_cwnd(line))            
        return traceEntries

if __name__ == '__main__':
    cwnd_arr = extract_cwnd()
    f = open("snd_cwnd.txt", "w")
    for cwnd in cwnd_arr:
        f.write(cwnd+"\n")
    f.close()
