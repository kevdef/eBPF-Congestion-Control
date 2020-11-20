import matplotlib.pyplot as plt
import numpy as np
import os
import re

def plot():
    snd_cwnd=[]
    with open ('data.txt', 'rt') as myfile:
        for line in myfile:
            if line.strip():
                snd_cwnd.append(float(line))
    times=[]
    with open ('times.txt', 'rt') as myfile:
        for line in myfile:
            if re.match(r'^-?\d+(?:\.\d+)?$', line) is not None:
                times.append(float(line))
    plt.scatter(times, snd_cwnd)

    plt.xlabel('time')
    plt.ylabel('snd_cwnd size')
    plt.title('Congestion Window Over Time')
    plt.grid(True)
    #Unique Name of Plot Image
    pathNameNum = 0
    while True:
        pathNameNum += 1
        newname = '{}{:d}.png'.format("cwnd_over_time", pathNameNum)
        newname = './cwnd-over-time/{}'.format(newname)
        if os.path.exists(newname):
            continue
        plt.savefig(newname)
        break
    plt.show()


if __name__ == '__main__':
    plot()

