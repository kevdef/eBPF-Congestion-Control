import matplotlib.pyplot as plt
import numpy as np
import os

def plot():
    snd_cwnd=[]
    with open ('snd_cwnd.txt', 'rt') as myfile:
        for line in myfile:
            snd_cwnd.append(float(line))

    time= list(range(0,len(snd_cwnd)*8,8))
    plt.plot(time, snd_cwnd)

    plt.xlabel('time (ms)')
    plt.ylabel('snd_cwnd size ')
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
def extract_cwnd():
    traceEntries=[]
    with open ('snd_cwnd.txt', 'rt') as myfile:
        for line in myfile:
            traceEntries.append(line)
        return traceEntries


if __name__ == '__main__':
    os.system("sudo su -c './get_trace.sh'")
    exec(open("ftrace_parser.py").read())
    plot()

    

