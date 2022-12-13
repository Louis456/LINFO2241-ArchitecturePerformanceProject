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
    print("make clean, make all done")

    for rate in REQUEST_RATES:
        print('----------------\nrequest_rate:', rate)
        server_proc = script_server()
        time.sleep(1)
        client_proc = script_client(rate)
        client_output = client_proc.communicate()[0].decode()
        server_output = server_proc.communicate()[0].decode()

        service_times = (float(stime) for stime in re.findall("service_time=(\d+.?\d*)", server_output))
        del server_output
        response_times = (float(rtime) for rtime in re.findall("response_time=(\d+.?\d*)", client_output))
        del client_output
        print("service_times:", np.mean(service_times), ", std:", np.std(service_times))
        print("response_times:", np.mean(response_times), ", std:", np.std(response_times))
        

    print("\nplotting graph...")
    fig, ax = plt.subplots()
    ax.hist(service_times,bins=50, density=True)
    

    ax.set_ylabel('probability')
    ax.set_title('probability distribution of service time [S]')
    ax.set_xlabel('service time (ms)')
    ax.legend()

    plt.ylim(bottom=0)
    plt.rc('axes', axisbelow=True)
    plt.grid(axis='y', linestyle='dashed')
    fig.tight_layout()
    plt.savefig("S_distribution.png")
    plt.close()

    print("done")