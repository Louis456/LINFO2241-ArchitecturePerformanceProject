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
KSIZES = (8, 128)
THREAD = '1'
OPTIS = (0, 1, 2, 3)
NB_ITERATIONS = 5
PARENT_PATH = Path().resolve().parent

def make_clean_make_all():
    subprocess.run(['make', 'clean'], cwd=PARENT_PATH)
    subprocess.run(['make', 'all'], cwd=PARENT_PATH)

def init_file(filename):
    if not os.path.exists('data'):
        os.makedirs('data')
    with open(filename, 'w') as file:
        file.write("fsize,ksize,request_rate,thread,opti,response_time\n")

def script_server(opti):
    if opti == 0:
        process = subprocess.Popen(['./server', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT, '-o', str(opti)], stdout=subprocess.PIPE, cwd=PARENT_PATH)
    elif opti == 1:
        process = subprocess.Popen(['./server-inline', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT, '-o', str(opti)], stdout=subprocess.PIPE, cwd=PARENT_PATH)
    elif opti == 2:
        process = subprocess.Popen(['./server-unroll', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT, '-o', str(opti)], stdout=subprocess.PIPE, cwd=PARENT_PATH)
    else:
        process = subprocess.Popen(['./server-optim', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT, '-o', str(opti)], stdout=subprocess.PIPE, cwd=PARENT_PATH)
    return process

def script_client(ksize):
    process = subprocess.Popen(['./client', '-k', str(ksize), '-r', REQUEST_RATE, '-t', DURATION, SERVER_IP+':'+SERVER_PORT], stdout=subprocess.PIPE, cwd=PARENT_PATH)
    return process


if __name__ == "__main__":
    make_clean_make_all()

    for ksize in KSIZES:
        print("\n=============\nKey size:", ksize)
        for opti in OPTIS:
            print("\n---------\nOpti:", opti)
            for it in range(NB_ITERATIONS):
                print("iteration:", it)

                server_proc = script_server(opti)
                time.sleep(1)
                client_proc = script_client(ksize)
                server_output = server_proc.communicate()[0].decode()
                client_output = client_proc.communicate()[0].decode()
                
                response_times = (float(rtime.split("=")[1]) for rtime in re.findall("response_time=\d+.?\d*", client_output))

                # Save measurements
                filename = "data/times_key_"+str(ksize)+"_opti_"+str(opti)+"_it_"+str(it)+".csv"
                init_file(filename)
                with open(filename, 'a') as file:
                    for rtime in response_times:
                        file.write("{0},{1},{2},{3},{4},{5}\n".format(FSIZE, ksize, REQUEST_RATE, THREAD, opti, rtime))


    
                        
                    
    
    
    
                    