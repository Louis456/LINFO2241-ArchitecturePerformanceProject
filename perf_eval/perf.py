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
REQUEST_RATE = '3'
FSIZE = '1024'
KSIZES = (8, 128)
THREAD = '1'
OPTIS = (0, 1, 2, 3)
NB_ITERATIONS = 1
PARENT_PATH = Path().resolve().parent
PERF_ARGS = ("cache-references","cache-misses","dTLB-loads","dTLB-load-misses","dTLB-stores","dTLB-store-misses","iTLB-loads","iTLB-load-misses","L1-dcache-loads","L1-dcache-load-misses","LLC-loads","LLC-load-misses","LLC-stores","LLC-store-misses")

def make_clean_make_all():
    subprocess.run(['make', 'clean'], cwd=PARENT_PATH)
    subprocess.run(['make', 'all'], cwd=PARENT_PATH)

def init_file(filename: str, headers: str):
    if not os.path.exists('data'):
        os.makedirs('data')
    with open(filename, 'w') as file:
        file.write(headers)

def script_server(opti=0):
    cmd = ['perf', 'stat', '-e']
    cmd.append(",".join(PERF_ARGS))
    if opti == 0:
        process = subprocess.Popen(cmd + ['./server', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=PARENT_PATH)
    elif opti == 1:
        process = subprocess.Popen(cmd + ['./server-inline', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=PARENT_PATH)
    elif opti == 2:
        process = subprocess.Popen(cmd + ['./server-unroll', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=PARENT_PATH)
    else:
        process = subprocess.Popen(cmd + ['./server-optim', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=PARENT_PATH)
    return process
    

def script_client(ksize: int):
    process = subprocess.Popen(['./client', '-k', str(ksize), '-r', REQUEST_RATE, '-t', DURATION, SERVER_IP+':'+SERVER_PORT], stdout=subprocess.PIPE, cwd=PARENT_PATH)
    return process


if __name__ == "__main__":
    make_clean_make_all()

    filename_perf = "data/perf_measurements.csv"
    init_file(filename_perf, "fsize,ksize,request_rate,thread,opti" + ','.join(PERF_ARGS) + "\n")

    for ksize in KSIZES:
        print("\n=============\nKey size:", ksize)
        for opti in OPTIS:
            print("\n---------\nOpti:", opti)
            filename_rtime = "data/rtime_key_"+str(ksize)+"_opti_"+str(opti)+".csv"
            init_file(filename_rtime, "fsize,ksize,request_rate,thread,opti,rtime,iteration\n")
            for it in range(NB_ITERATIONS):
                print("iteration:", it)
                ## Perf stat measurements ##
                server_proc = script_server(opti)
                time.sleep(1)
                client_proc = script_client(ksize)
                server_output = server_proc.communicate()[1].decode()
                client_output = client_proc.communicate()[0].decode()

                response_times = (float(rtime) for rtime in re.findall("response_time=(\d+.?\d*)", client_output))

                measurements = {} # {perf_arg: value}
                for perf_arg in PERF_ARGS:
                    measurements[perf_arg] = float(re.search("(\d+(?:.?\d)*)\s+"+perf_arg, server_output).group(1).replace(".", ""))
                
                # Save response times to file
                with open(filename_rtime, 'a') as file:
                    for rt in response_times:
                        file.write("{0},{1},{2},{3},{4},{5}\n".format(FSIZE, ksize, REQUEST_RATE, THREAD, opti, rt, it))
                
                # Save perf measurements to file
                with open(filename_perf, 'a') as file:
                    file.write("{0},{1},{2},{3},{4}".format(FSIZE, ksize, REQUEST_RATE, THREAD, opti))
                    for perf_arg in PERF_ARGS:
                        file.write(",{0}".format(measurements[perf_arg]))
                    file.write("\n")
    
                



    
                        
                    
    
    
    
                    