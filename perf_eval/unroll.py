#!/usr/bin/env python

import os
import numpy as np
import subprocess
import time
import re
from datetime import datetime
from pathlib import Path


SERVER_IP = "127.0.0.1"
SERVER_PORT = "2241"
DURATION = '10' # seconds
REQUEST_RATE = '10'
FSIZE = '1024'
KSIZE = '128'
THREAD = '1'
NB_ITERATIONS = 10
PARENT_PATH = Path().resolve().parent


def make_clean_make_all():
    subprocess.run(['make', 'clean'], cwd=PARENT_PATH)
    subprocess.run(['make', 'all'], cwd=PARENT_PATH)

def init_file(filename: str, headers: str):
    if not os.path.exists('data'):
        os.makedirs('data')
    with open(filename, 'w') as file:
        file.write(headers)

def script_server():
    
    
    process = subprocess.Popen(['./server-optim', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=PARENT_PATH)
    return process
    

def script_client():
    process = subprocess.Popen(['./client', '-k', KSIZE, '-r', REQUEST_RATE, '-t', DURATION, SERVER_IP+':'+SERVER_PORT], stdout=subprocess.PIPE, cwd=PARENT_PATH)
    return process

if __name__ == "__main__":

    filename = "data/unroll_measurements.csv"
    init_file(filename, "unroll,rtt,fsize,ksize,request_rate,thread,iteration\n")

    for it in range(NB_ITERATIONS):
        server_proc = script_server()
        time.sleep(1)
        client_proc = script_client()
        server_output = server_proc.communicate()[1].decode()
        client_output = client_proc.communicate()[0].decode()

        response_times = (float(rtime) for rtime in re.findall("response_time=(\d+.?\d*)", client_output))
        unroll = re.search("unroll (\d+)", client_output).group(1)

        with open(filename, 'a') as file:
            for rt in response_times:
                file.write("{0},{1},{2},{3},{4},{5}\n".format(unroll, rt, FSIZE, KSIZE, REQUEST_RATE, THREAD, it))

        
    "rtt, unroll"

    "2323,8"