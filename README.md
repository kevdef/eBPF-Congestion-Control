# eBPF-Congestion-Control
Developing a framework using eBPF to help developers write congestion control programs for TCP networks.

<details>
           <summary>bpf_aimd.c</summary>
           <p>*A script to employ a simple additive increase multiplicative decrease as congestion control. </p>
           <p>*To compile, move file to /linux/tools/testing/selftests/bpf/progs.
           <p>*Go to /linux/tools/testing/selftests/bpf, and run 'make'.
           <p>*The file should have succesful created a .o file within .../bpf
           <p>*Use the command 'bpftool struct_ops register bpf_aimd.o'
           <p>*Now use the command 'sysctl -w net.ipv4.tcp_congestion_control=bpf_aimd'
           <p> The program should now be the default congestion control algorithm.
            
</details>

