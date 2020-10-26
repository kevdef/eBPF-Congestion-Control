import sys
import os

def main():
    while True:
        text = input("(0)Exit program\n(1)Run Iperf Sequence + Get Trace\n(2)Trace to Data\n(3)Plot Graph\n")  # Python 3
        if int(text)==0:
            break
        elif int(text)==1:
            os.system("sudo su -c './get_trace.sh'")
        elif int(text)==2:
            os.system("sudo su -c './trace_to_data.sh'")
        elif int(text)==3:
            os.system("python3 graph_plotter.py")

if __name__ == "__main__":
    main()

