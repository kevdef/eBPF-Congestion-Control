import matplotlib.pyplot as plt
import numpy as np
import os

def plot():
    snd_cwnd=[]
    with open ('data.txt', 'rt') as myfile:
        for line in myfile:
            snd_cwnd.append(float(line))
    times=[]
    with open ('times.txt', 'rt') as myfile:
        for line in myfile:
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
        if os.path.exists(newname):
            continue
        plt.savefig(newname)
        break
    plt.show()


if __name__ == '__main__':
    plot()

