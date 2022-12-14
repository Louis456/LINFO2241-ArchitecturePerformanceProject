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
DURATION = '30' # seconds
FSIZE = '1024'
KSIZE = '128'
THREAD = '1'
REQUEST_RATES = (25,50,100,150,200)
THEORY_VALUES = (42,)

def make_clean_make_all():
    subprocess.run(['make', 'clean'])
    subprocess.run(['make', 'client'])
    subprocess.run(['make', 'server-float-avx'])

def script_server():
    return subprocess.Popen(['./server-float-avx', '-j', THREAD, '-s', FSIZE, '-p', SERVER_PORT], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    
def script_client(request_rate: int):
    return subprocess.Popen(['./client', '-k', KSIZE, '-r', str(request_rate), '-t', DURATION, SERVER_IP+':'+SERVER_PORT], stdout=subprocess.PIPE)

def barplot_theory(xs, ys, labels, stds):
    fig, ax = plt.subplots() #figsize=(10, 7)
    width = 0.1
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


if __name__ == "__main__":
    make_clean_make_all()
    print("make clean, make all done")

    ys_rtt = []
    stds_rtt = []
    s_times = []

    for rate in REQUEST_RATES:
        print('----------------\nrequest_rate:', rate)
        server_proc = script_server()
        time.sleep(1)
        client_proc = script_client(rate)
        client_output = client_proc.communicate()[0].decode()
        server_output = server_proc.communicate()[0].decode()

        service_times = np.array(list(float(stime)/1000.0 for stime in re.findall("service_time=(\d+.?\d*)", server_output)))
        del server_output
        response_times = np.array(list(float(rtime)/1000.0 for rtime in re.findall("response_time=(\d+.?\d*)", client_output)))
        del client_output
        print("service_times:", np.mean(service_times), ", std:", np.std(service_times))
        print("service times :",service_times)
        #print("response_times:", np.mean(response_times), ", std:", np.std(response_times))

        ys_rtt.append(np.mean(response_times))
        stds_rtt.append(np.std(response_times))


        """"
        f = Fitter(service_times)
        f.fit()
        # make take some time since by default, all distribution are tried
        f.summary()
        plt.show()
        f.get_best()
        """
        s_times.extend(service_times)

        print("\nplotting graph...")
        sns.histplot(data=service_times, stat="probability")
        x = np.linspace(np.min(service_times),np.max(service_times))
        y_norm = ss.norm.pdf(x, np.mean(service_times), np.std(service_times)) # the normal pdf
        y_exp = ss.expon.pdf(x, np.mean(service_times), np.std(service_times))
        y_chi2 = ss.chi2.pdf(x, np.mean(service_times))
        alpha = (np.mean(service_times)*np.mean(service_times))/np.var(service_times)
        beta = np.mean(service_times)/np.var(service_times)
        y_gamma = ss.gamma.pdf(x, alpha,beta)
        
        
        plt.plot(x, y_norm, color="red", label=f"norm µ={np.mean(service_times):.2f}, σ={np.std(service_times):.2f}")
        plt.plot(x, y_exp, color='green', label=f"exp µ={np.mean(service_times):.2f}, σ={np.std(service_times):.2f}")
        plt.plot(x, y_chi2, color='purple', label=f"chi2 df={np.mean(service_times):.2f}")
        plt.plot(x, y_gamma, color='yellow', label=f"gamma α={alpha:.2f}, β={beta:.2f}")

        plt.title('probability distribution of service time [S]')
        plt.xlabel('service time (ms)')
        plt.legend()

        plt.ylim(bottom=0)
        plt.rc('axes', axisbelow=True)
        plt.grid(axis='y', linestyle='dashed')
        #plt.xlim(xmin=0)
        if not os.path.exists("perf_eval"):
            os.makedirs("perf_eval")
        if not os.path.exists("perf_eval/plots_phase4"):
            os.makedirs("perf_eval/plots_phase4")
        plt.savefig(f"perf_eval/plots_phase4/S_distribution_rate_{rate}.png")
        plt.close()

        print("done")

    print("\nplotting graph...")
    sns.histplot(data=s_times, stat="probability")
    x = np.linspace(np.min(s_times),np.max(s_times))
    y_norm = ss.norm.pdf(x, np.mean(s_times), np.std(s_times)) # the normal pdf
    y_exp = ss.expon.pdf(x, np.mean(s_times), np.std(s_times))
    y_chi2 = ss.chi2.pdf(x, np.mean(s_times))
    alpha = (np.mean(s_times)*np.mean(s_times))/np.var(s_times)
    beta = np.mean(s_times)/np.var(s_times)
    y_gamma = ss.gamma.pdf(x, alpha,beta)
    
    """
    plt.plot(x, y_norm, color="red", label=f"norm µ={np.mean(s_times):.2f}, σ={np.std(s_times):.2f}")
    plt.plot(x, y_exp, color='green', label=f"exp µ={np.mean(s_times):.2f}, σ={np.std(s_times):.2f}")
    plt.plot(x, y_chi2, color='purple', label=f"chi2 df={np.mean(s_times):.2f}")
    plt.plot(x, y_gamma, color='yellow', label=f"gamma α={alpha:.2f}, β={beta:.2f}")
    """

    plt.title('probability distribution of service time [S]')
    plt.xlabel('service time (ms)')
    plt.legend()

    plt.ylim(bottom=0)
    plt.rc('axes', axisbelow=True)
    plt.grid(axis='y', linestyle='dashed')
    #plt.xlim(xmin=0)
    if not os.path.exists("perf_eval"):
        os.makedirs("perf_eval")
    if not os.path.exists("perf_eval/plots_phase4"):
        os.makedirs("perf_eval/plots_phase4")
    plt.savefig(f"perf_eval/plots_phase4/S_distribution_rate.png")
    plt.close()

    print("done")


    ys = (ys_rtt, THEORY_VALUES)
    stds = (stds_rtt, [])
    labels = ("real", "theory")
    barplot_theory(REQUEST_RATES, ys, labels, stds)

    



    print("All done !")