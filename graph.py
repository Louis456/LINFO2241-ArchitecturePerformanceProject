#!/usr/bin/env python

import numpy as np
import subprocess
import time
import re
from pathlib import Path
import matplotlib.pyplot as plt


SERVER_IP = "127.0.0.1"
SERVER_PORT = "2244"
DURATION = '5' # seconds
REQUEST_RATES = (8, 16, 32, 64, 128, 256, 512, 1024)
FSIZE = '1024'
KSIZE = '128'
THREAD = '1'

def make_clean_make_all():
    subprocess.run(['make', 'clean'])
    subprocess.run(['make', 'client'])
    subprocess.run(['make', 'server-float-avx'])

def script_server():
    return subprocess.Popen(['./server-float-avx', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    
def script_client(request_rate: int):
    return subprocess.Popen(['./client', '-k', KSIZE, '-r', str(request_rate), '-t', DURATION, SERVER_IP+':'+SERVER_PORT], stdout=subprocess.PIPE)


if __name__ == "__main__":
    make_clean_make_all()

    for rate in REQUEST_RATES:
        server_proc = script_server()
        time.sleep(1)
        client_proc = script_client(rate)
        client_output = client_proc.communicate()[0].decode()

        server_lines = []
        server_output = ""
        try:
            for line in server_proc.stdout:
                server_lines.append(line)
        finally:
            server_proc.terminate()
            server_output = "".join(server_lines)
            del server_lines

        service_times = (float(stime) for stime in re.findall("service_time=(\d+.?\d*)", server_output))
        del server_output
        response_times = (float(rtime) for rtime in re.findall("response_time=(\d+.?\d*)", client_output))
        del client_output
        

    fig, ax = plt.subplots()
    ax.hist(service_times,bins=20, density=True)
    

    ax.set_ylabel('probability')
    ax.set_title('probability distribution of service time S')
    ax.set_xlabel('service time')
    ax.legend()

    plt.ylim(bottom=0)
    plt.rc('axes', axisbelow=True)
    plt.grid(axis='y', linestyle='dashed')
    fig.tight_layout()
    plt.savefig("S_distribution.png")
    plt.close()