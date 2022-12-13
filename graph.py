#!/usr/bin/env python

import numpy as np
import subprocess
import time
import re
from pathlib import Path
import matplotlib.pyplot as plt
import seaborn as sns
import os


SERVER_IP = "127.0.0.1"
SERVER_PORT = "2244"
DURATION = '30' # seconds
REQUEST_RATES = (32, 64)
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

        service_times = np.array(list(float(stime) for stime in re.findall("service_time=(\d+.?\d*)", server_output)))
        del server_output
        response_times = np.array(list(float(rtime) for rtime in re.findall("response_time=(\d+.?\d*)", client_output)))
        del client_output

        print("service_times:", np.mean(service_times), ", std:", np.std(service_times))
        print("response_times:", np.mean(response_times), ", std:", np.std(response_times))

        print("\nplotting graph...")
        sns.histplot(data=service_times)

        plt.title('probability distribution of service time [S]')
        plt.xlabel('service time (ms)')
        #ax.legend()

        plt.ylim(bottom=0)
        plt.rc('axes', axisbelow=True)
        plt.grid(axis='y', linestyle='dashed')
        if not os.path.exists("perf_eval"):
            os.makedirs("perf_eval")
        if not os.path.exists("perf_eval/plots_phase4"):
            os.makedirs("perf_eval/plots_phase4")
        plt.savefig(f"perf_eval/plots_phase4/S_distribution_rate_{rate}.png")
        plt.close()

        print("done")