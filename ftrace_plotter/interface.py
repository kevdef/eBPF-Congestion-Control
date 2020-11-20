import sys
import os

def main():
    while True:
        text = input("(0)Exit program\n(1)Run Iperf Sequence + Get Trace\n(2)Trace to Data\n(3)Plot Graph\n(4)Choose cng_cntrl\n")  # Python 3
        if int(text)==0:
            break
        elif int(text)==1:
            os.system("sudo su -c './get_trace.sh'")
        elif int(text)==2:
            os.system("sudo su -c './trace_to_data.sh'")
        elif int(text)==3:
            os.system("python3 graph_plotter.py")
        elif int(text)==4:
            while True:
                text = input("(0)Exit cng_cntrl menu\n(1)K-Cubic\n(2)K-Reno\n(3)Bpf-Cubic\n")
                if int(text)==0:
                    break
                elif int(text)==1:
                    os.system("sudo sysctl -w net.ipv4.tcp_congestion_control=cubic")
                elif int(text)==2:
                    os.system("sudo sysctl -w net.ipv4.tcp_congestion_control=reno")
                elif int(text)==3:
                    os.system("sudo sysctl -w net.ipv4.tcp_congestion_control=bpf_cubic")


if __name__ == "__main__":
    main()

