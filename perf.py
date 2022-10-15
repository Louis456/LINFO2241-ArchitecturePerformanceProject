#!/usr/bin/env python
import os
import numpy as np
import matplotlib.pyplot as plt
import subprocess
import time
import asyncio

SERVER_IP = "127.0.0.1"
SERVER_PORT = "2241"
TIME = 5

def init():
    subprocess.run("make clean")
    subprocess.run("make all")

async def script_server(thread, fsize):
    proc = await subprocess.check_output()
    return proc

async def script_client(ksize, request_rate):
    proc = await subprocess.check_output()
    return proc

if __name__ == "__main__":
    FSIZES = (128,)
    KSIZES = (32,)
    REQUEST_RATES = (150,)
    THREADS = (2,)

    init()

    for fsize in FSIZES:
        for ksize in KSIZES:
            for request_rate in REQUEST_RATES:
                for thread in THREADS:
                    server_proc = script_server(thread, fsize)
                    time.sleep(5)
                    client_proc = script_client(ksize, request_rate)
                    server_proc.wait()
                    client_proc.wait()





    if not os.path.exists('plots'):
        os.makedirs('plots')