#!/usr/bin/env python

import numpy as np
import subprocess
import time
import re
from pathlib import Path
import matplotlib.pyplot as plt
import seaborn as sns
import os
import scipy.stats as ss
from fitter import Fitter


SERVER_IP = "127.0.0.1"
SERVER_PORT = "2244"
DURATION = ('60','40','30','24','20','17','15','13','12','11','10') # seconds
FSIZE = '1024'
KSIZE = '128'
THREAD = '1'
REQUEST_RATES = (20,30,40,50,60,70,80,90,100,110,120)
TRUE_VALUES_MEAN = (27.00,21.64,22.23,22.32,17.55,21.93,23.97,28.77,30.00,36.72,199.92)
TRUE_VALUES_STD = (14.44,13.61,13.71,14.74,11.82,15.69,14.49,18.50,20.79,26.97,81.06)
THEORY_VALUES = (13.91, 15.74, 18.36, 22.39, 29.42, 44.84, 105.54, 0, 0, 0, 0)

def make_clean_make_all():
    subprocess.run(['make', 'clean'])
    subprocess.run(['make', 'client-queue'])
    subprocess.run(['make', 'server-queue'])

def script_server():
    return subprocess.Popen(['./server-queue', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    
def script_client(request_rate: int, duration:str):
    return subprocess.Popen(['./client-queue', '-k', KSIZE, '-r', str(request_rate), '-t', duration, SERVER_IP+':'+SERVER_PORT], stdout=subprocess.PIPE)

def barplot_theory(xs, ys, labels, stds):
    fig, ax = plt.subplots() #figsize=(10, 7)
    width = 0.3
    x = np.arange(len(xs))
    nb_bars = len(ys)
    v = - (nb_bars-1) / 2
    for i in range(nb_bars):
        if stds[i] == []:
            ax.bar(x + width * (v + i), ys[i], width, label=labels[i], alpha=0.95, capsize=4)
        else:
            ax.bar(x + width * (v + i), ys[i], width, label=labels[i], yerr=stds[i], alpha=0.95, capsize=4)
        

    ax.set_ylabel("mean RTT (ms)")
    ax.set_xlabel("request rate")
    ax.legend()
    ax.set_title("Comparison of the mean RTT with the theoretical value for various request rates")
    ax.set_xticks(x, labels=xs)
    ax.set_axisbelow(True)
    #plt.xticks(rotation=70)
    plt.grid(axis='y', linestyle='dashed')
    fig.tight_layout()
    plt.savefig("perf_eval/plots_phase4/barplot_theory.png")
    plt.close()

def histplot_S_distribution(xs, filename, fit=False):
    sns.histplot(data=xs, stat="probability")
    x = np.linspace(np.min(xs),np.max(xs))
    y_norm = ss.norm.pdf(x, np.mean(xs), np.std(xs)) # the normal pdf
    y_exp = ss.expon.pdf(x, np.mean(xs), np.std(xs))
    y_chi2 = ss.chi2.pdf(x, np.mean(xs))
    alpha = (np.mean(xs)*np.mean(xs))/np.var(xs)
    beta = np.mean(xs)/np.var(xs)
    y_gamma = ss.gamma.pdf(x, alpha,beta)
    
    if fit==True:
        plt.plot(x, y_norm, color="red", label=f"norm µ={np.mean(xs):.2f}, σ={np.std(xs):.2f}")
        plt.plot(x, y_exp, color='green', label=f"exp µ={np.mean(xs):.2f}, σ={np.std(xs):.2f}")
        plt.plot(x, y_chi2, color='purple', label=f"chi2 df={np.mean(xs):.2f}")
        plt.plot(x, y_gamma, color='yellow', label=f"gamma α={alpha:.2f}, β={beta:.2f}")
        

    plt.title('probability distribution of service time [S]')
    plt.xlabel('service time (ms)') 
    plt.legend()

    plt.ylim(bottom=0)
    plt.rc('axes', axisbelow=True)
    plt.grid(axis='y', linestyle='dashed')
    if not os.path.exists("perf_eval"):
        os.makedirs("perf_eval")
    if not os.path.exists("perf_eval/plots_phase4"):
        os.makedirs("perf_eval/plots_phase4")
    plt.savefig(f"perf_eval/plots_phase4/{filename}.png")
    plt.close() 


if __name__ == "__main__":
    make_clean_make_all()
    print("make clean, make all done")

    ys_rtt = []
    stds_rtt = []
    s_times = []
    with open('queuing.csv', 'w') as csvfile:
        csvfile.write("rate,s_mean,s_std,r_mean,r_std\n")
        
        for i in range(len(REQUEST_RATES)):
            print('----------------\nrequest_rate:', REQUEST_RATES[i])
            server_proc = script_server()
            time.sleep(1)
            client_proc = script_client(REQUEST_RATES[i], DURATION[i])
            client_output = client_proc.communicate()[0].decode()
            server_output = server_proc.communicate()[0].decode()

            service_times = np.array(list(float(stime)/1000.0 for stime in re.findall("service_time=(\d+.?\d*)", server_output)))
            del server_output
            response_times = np.array(list(float(rtime) for rtime in re.findall("response_time=(\d+.?\d*)", client_output)))
            del client_output
            print("service_times:", np.mean(service_times), ", std:", np.std(service_times))
            print("response_times:", np.mean(response_times), ", std:", np.std(response_times))
            csvfile.write(f"{REQUEST_RATES[i]},{np.mean(service_times)},{np.std(service_times)},{np.mean(response_times)},{np.std(response_times)}\n")
            ys_rtt.append(np.mean(response_times))
            stds_rtt.append(np.std(response_times))

            s_times.extend(service_times)
        
            print("\nplotting graph...")
            histplot_S_distribution(service_times, f"S_distribution_rate_{REQUEST_RATES[i]}.png",fit=False)
            histplot_S_distribution(service_times, f"S_distribution_rate_{REQUEST_RATES[i]}_fit.png",fit=True)
            print("done")

        s_times = np.array(s_times)
        print("service_times all:", np.mean(s_times), ", std:", np.std(s_times))
        print("service times all :",s_times)
        csvfile.write(f"ALL,{np.mean(s_times)},{np.std(s_times)},NaN,NaN\n")
        print("\nplotting graph...")
        histplot_S_distribution(s_times, "S_distribution_rate.png",fit=False)
        histplot_S_distribution(s_times, "S_distribution_rate_fit.png",fit=True)
        
    print("done")
    ys = (TRUE_VALUES_MEAN, THEORY_VALUES)
    stds = (TRUE_VALUES_STD, [])
    labels = ("experimental", "theoretical")
    barplot_theory(REQUEST_RATES, ys, labels, stds)

    print("All done !")