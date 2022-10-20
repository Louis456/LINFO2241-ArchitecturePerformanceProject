#!/usr/bin/env python
import os
import numpy as np
import subprocess
import time
import re
from datetime import datetime
from pathlib import Path



SERVER_IP = "10.0.1.3"
SERVER_PORT = "2241"
DURATION = 10 # seconds
DAY_TIME = datetime.now().strftime("%d_%m_%Y_%H_%M_%S")
FILENAME_THROUGHPUT = "data/"+DAY_TIME+"_throughput.csv"
FILENAME_RESPONSE_TIME = "data/"+DAY_TIME+"_response_time.csv"
#PARENT_PATH = Path().parent.absolute()
PARENT_PATH = Path().resolve().parent

def init():
    # Create the files in which the results will be stored
    if not os.path.exists('data'):
        os.makedirs('data')

    with open(FILENAME_THROUGHPUT, 'w') as file:
        file.write("fsize,ksize,request_rate,thread,throughput\n")
    
    with open(FILENAME_RESPONSE_TIME, 'w') as file:
        file.write("fsize,ksize,request_rate,thread,response_time\n")
    
    subprocess.run(['make', 'clean'], cwd=PARENT_PATH)
    subprocess.run(['make', 'all'], cwd=PARENT_PATH)


def script_server(thread, fsize):
    process = subprocess.Popen(['ssh', 'server', '/root/LINFO2241-ArchitecturePerformanceProject/./server', '-j', str(thread), '-s', str(fsize), '-p', SERVER_PORT], stdout=subprocess.PIPE, cwd=PARENT_PATH)
    return process


def script_client(ksize, request_rate):
    process = subprocess.Popen(['./client', '-k', str(ksize), '-r', str(request_rate), '-t', str(DURATION), SERVER_IP+':'+SERVER_PORT], stdout=subprocess.PIPE, cwd=PARENT_PATH)
    return process    


if __name__ == "__main__":
    #2^k e
    FSIZES = (128,256)
    KSIZES = (32,64)
    REQUEST_RATES = (150,300)
    THREADS = (2,4)
    NB_ITERATION = 1

    init()

    it = 1

    for fsize in FSIZES:
        for ksize in KSIZES:
            for request_rate in REQUEST_RATES:
                for thread in THREADS:
                    throughput_bytes = np.empty(NB_ITERATION, dtype=float)
                    throughput_packets = np.empty(NB_ITERATION, dtype=float)
                    response_time = np.empty(NB_ITERATION, dtype=float)
                    for i in range(NB_ITERATION):
                        server_proc = script_server(thread, fsize)
                        time.sleep(4)
                        client_proc = script_client(ksize, request_rate)

                        server_output = server_proc.communicate()[0].decode()
                        client_output = client_proc.communicate()[0].decode()
                        #print("client output : ", client_output)
                        print(it)
                        it += 1
                        #print("server output : ", server_output)

                        
                        throughput_bytes[i] = float(re.search(r"mean\sthroughput\sbytes\s(\d+.?\d*)", client_output).group(1))
                        throughput_packets[i] = float(re.search(r"mean\sthroughput\spackets\s(\d+.?\d*)", client_output).group(1))
                        response_time[i] = float(re.search(r"mean\sresponse\stime\s(\d+.?\d*)", client_output).group(1))


                        # Save results to files
                        with open(FILENAME_THROUGHPUT, 'a') as file:
                            file.write("{0},{1},{2},{3},{4}\n".format(fsize, ksize, request_rate, thread, throughput_packets[i]))
                        with open(FILENAME_RESPONSE_TIME, 'a') as file:
                            file.write("{0},{1},{2},{3},{4}\n".format(fsize, ksize, request_rate, thread, response_time[i]))
                                                                      
                        
                    mean_throughput_bytes = np.mean(throughput_bytes)
                    mean_throughput_packets = np.mean(throughput_packets)
                    mean_response_time = np.mean(response_time)
                    std_throughput_bytes = np.std(throughput_bytes)
                    std_throughput_packets = np.std(throughput_packets)
                    std_response_time = np.std(response_time)
                    

                    """
                    with open(FILENAME_THROUGHPUT, 'a') as file:
                        file.write("{0},{1},{2},{3},{4}\n".format(fsize, ksize, request_rate, thread, mean_throughput_packets))
                    with open(FILENAME_RESPONSE_TIME, 'a') as file:
                        file.write("{0},{1},{2},{3},{4}\n".format(fsize, ksize, request_rate, thread, mean_response_time))
                    """
    
    
    
                    