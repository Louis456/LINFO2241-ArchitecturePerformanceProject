#!/usr/bin/env python
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sn
import os


KSIZES = (8, 128)
OPTIS = (0, 1, 2, 3)
FILENAME_PERF = "data/perf_measurements.csv"
FILENAME_UNROLL = "data/unroll_measurements.csv"
FILENAMES_RTIME = ["data/rtime_key_"+str(ksize)+"_opti_"+str(opti)+".csv" for ksize in KSIZES for opti in OPTIS]
PLOTS_DIRECTORY = "plots_phase2"
OPTI_NAMES = ("None", "Line by line", "Unroll", "Both")


def boxplot_rtime(data_8, data_128, labels_8, labels_128, out_filename, type="split"):
    if type == "split":
        fig, (ax1, ax2) = plt.subplots(1, 2)
        fig.suptitle("Response times with respect to key size and optimization")
        bp = ax1.boxplot(data_8)
        bp = ax2.boxplot(data_128)
        fig.set_figheight(9)
        fig.set_figwidth(16)
        ax1.set_xticks(np.arange(1, len(labels_8)+1), [label for label in labels_8], rotation=75)
        ax2.set_xticks(np.arange(1, len(labels_128)+1), [label for label in labels_128], rotation=75)
        ax1.set_ylim(bottom=0)
        ax2.set_ylim(bottom=0)
        ax1.set_ylabel("Response time (ms)")
        ax2.set_ylabel("Response time (ms)")
        ax1.grid(axis='y', linestyle='dashed')
        ax2.grid(axis='y', linestyle='dashed')
        fig.subplots_adjust(bottom=0.3)
        fig.savefig(PLOTS_DIRECTORY+"/"+out_filename)
    else:
        fig, ax = plt.subplots()
        plt.title("Response times with respect to key size and optimization")
        bp = ax.boxplot(data_8 + data_128)
        fig.set_figheight(9)
        fig.set_figwidth(8)
        labels = labels_8 + labels_128
        ax.set_xticks(np.arange(1, len(labels)+1), [label for label in labels], rotation=75)
        ax.set_ylim(bottom=0)
        ax.set_ylabel("Response time (ms)")
        ax.grid(axis='y', linestyle='dashed')
    fig.subplots_adjust(bottom=0.3)
    fig.savefig(PLOTS_DIRECTORY+"/"+out_filename)

def barplot_multiple_bars(xs, ys, stds, labels, ylabel, title, out_filename, legend_loc='upper left'):
    fig, ax = plt.subplots(figsize=(10, 7))
    width = 0.1
    x = np.arange(len(xs))
    nb_bars = len(ys)
    v = - (nb_bars-1) / 2
    for i in range(nb_bars):
        ax.bar(x + width * (v + i), ys[i], width, label=labels[i], yerr=stds[i], alpha=0.95, capsize=4)

    ax.set_ylabel(ylabel)
    ax.legend(loc=legend_loc)
    
    ax.set_title(title)
    ax.set_xticks(x, labels=xs)
    ax.set_axisbelow(True)
    plt.xticks(rotation=70)
    plt.grid(axis='y', linestyle='dashed')
    fig.tight_layout()
    plt.savefig(PLOTS_DIRECTORY+"/"+out_filename)
    plt.close()

def barplot_single(xs, ys, stds, label, ylabel, title, out_filename, legend_loc='upper left'):
    fig  = plt.figure()
    fig.set_figheight(10)
    fig.set_figwidth(9)
    x = np.arange(len(xs))
    plt.bar(x, ys, label=label, yerr=stds, capsize=4)
    plt.ylabel(ylabel)
    plt.xticks(x, labels=xs)
    plt.xticks(rotation=70)
    plt.title(title)
    plt.legend(loc=legend_loc)
    plt.grid(axis='y', linestyle='dashed')
    plt.rc('axes', axisbelow=True)
    #plt.ticklabel_format(style='plain', axis='y')
    fig.subplots_adjust(bottom=0.3)
    plt.savefig(PLOTS_DIRECTORY+"/"+out_filename)
    plt.close()

def boxplot_single(xs, ys, ylabel, title, out_filename):
    fig, ax = plt.subplots()
    fig.set_figheight(10)
    fig.set_figwidth(9)
    ax.boxplot(ys, labels=xs)
    ax.set_ylabel(ylabel)
    ax.set_ylim(bottom=0)
    plt.xticks(rotation=80)
    plt.title(title)
    plt.grid(axis='y', linestyle='dashed')
    plt.rc('axes', axisbelow=True)
    fig.subplots_adjust(bottom=0.3)
    plt.savefig(PLOTS_DIRECTORY+"/"+out_filename)
    plt.close()

    

if __name__ == "__main__":
    if not os.path.exists(PLOTS_DIRECTORY):
        os.makedirs(PLOTS_DIRECTORY)
    if not os.path.exists(PLOTS_DIRECTORY+"/barplots"):
        os.makedirs(PLOTS_DIRECTORY+"/barplots")
    if not os.path.exists(PLOTS_DIRECTORY+"/boxplots"):
        os.makedirs(PLOTS_DIRECTORY+"/boxplots")


    ############################
    #### Correlation matrix ####
    ############################
    df = pd.read_csv(FILENAMES_RTIME[0])
    for i in range(1,len(FILENAMES_RTIME)):
        df_tmp = pd.read_csv(FILENAMES_RTIME[i])
        df = pd.concat([df, df_tmp], ignore_index=True, sort=True)
    corr_rtime = df.corr()
    print(corr_rtime.head())
    fig, ax = plt.subplots(figsize=(3, 6))
    sn.heatmap(corr_rtime[["rtime"]], annot=True)
    plt.margins(5)
    plt.savefig("plots_phase2/rtime_correlation.png")
    plt.close()


    ########################
    #### Response times ####
    ########################
    data_8 = []
    data_128 = []
    labels_8 = []
    labels_128 = []
    for filename in FILENAMES_RTIME:
        df = pd.read_csv(filename)
        print("\n\n", filename, "\n", df.describe())
        if (df.iloc[0]["ksize"] == 128):
            labels_128.append("Optim="+OPTI_NAMES[int(df.iloc[0]["opti"])]+ ", " + "ksize="+str(df.iloc[0]["ksize"]))
            data_128.append(np.array(df["rtime"].tolist()) / 1000)
        else:
            labels_8.append("Optim="+OPTI_NAMES[int(df.iloc[0]["opti"])]+ ", " + "ksize="+str(df.iloc[0]["ksize"]))
            data_8.append(np.array(df["rtime"].tolist()) / 1000)
    boxplot_rtime(data_8, data_128, labels_8, labels_128, "2k_rtime_plot_split.png")
    boxplot_rtime(data_8, data_128, labels_8, labels_128, "2k_rtime_plot_single.png", "single")


    ###########################
    ###### Unroll size ########
    ###########################
    df = pd.read_csv(FILENAME_UNROLL)
    unrolls = (1, 2, 4, 8, 16, 32, 64)
    xs = ["ksize=128, opti=Both, Unroll="+str(unroll) for unroll in unrolls]
    ys = [np.array(df['rtt'][df["unroll"] == unroll].tolist())/1000 for unroll in unrolls ]
    title = "Response time by the unrolling factor"
    ylabel = "Response time (ms)"
    boxplot_single(xs, ys, ylabel, title, "unroll_measurements.png")




    ###########################
    #### Perf measurements ####
    ###########################
    df = pd.read_csv(FILENAME_PERF)

    # Percentage of misses
    print("\n\n\n##### Percentage #####")
    labels = ('cache-misses', 'dTLB-load-misses', 'dTLB-store-misses', 'L1-dcache-load-misses', 'LLC-load-misses', 'LLC-store-misses') #'iTLB-load-misses'
    xs = []
    ys = [[] for i in range(len(labels))]
    stds = [[] for i in range(len(labels))]
    values = [[] for i in range(len(labels))]
    for ksize in KSIZES:    
        for opti in OPTIS:
            print("\nMean percentage of misses for ksize="+str(ksize)+" , opti="+OPTI_NAMES[opti]+" : ")
            xs.append("Optim="+OPTI_NAMES[opti]+ ", " + "ksize="+str(ksize))
            for i, label in enumerate(labels):
                load_name = label[:-7] + "s" if label != "cache-misses" else "cache-references"
                loads = np.array(df[load_name][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
                misses = np.array(df[label][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
                percentage = (np.array(misses) / loads) * 100
                print((label+":").ljust(30) + str(np.mean(percentage)).ljust(30) + "std: "+str(np.std(percentage)))
                values[i].append(percentage)
                ys[i].append(np.mean(percentage))
                stds[i].append(np.std(percentage)) 
    ylabel = "Misses (%)"
    # single plot
    for i, label in enumerate(labels):
        title="Mean percentage of "+label+" for every optimization levels and key sizes"
        out_filename = "barplots/cache_misses_percentage_"+label+".png"
        barplot_single(xs, ys[i], stds[i], label, ylabel, title, out_filename)
        title="Percentage of "+label+" for every optimization levels and key sizes"
        out_filename = "boxplots/cache_misses_percentage_"+label+".png"
        boxplot_single(xs, values[i], ylabel, title, out_filename)
    # multi plot
    title = "Mean percentage of misses for Perf metrics for every optimization levels and key sizes"
    barplot_multiple_bars(xs, ys, stds, labels, ylabel, title, "cache_misses_percentage.png", "upper right")


    # Total number of misses
    print("\n\n\n##### Misses #####")
    labels = ('cache-misses', 'dTLB-load-misses', 'dTLB-store-misses', 'iTLB-load-misses', 'L1-dcache-load-misses', 'LLC-load-misses', 'LLC-store-misses')
    xs = []
    ys = [[] for i in range(len(labels))]
    stds = [[] for i in range(len(labels))]
    values = [[] for i in range(len(labels))]
    for ksize in KSIZES:    
        for opti in OPTIS:
            print("\nMean number of misses for ksize="+str(ksize)+" , opti="+OPTI_NAMES[opti]+" : ")
            xs.append("Optim="+OPTI_NAMES[opti]+ ", " + "ksize="+str(ksize))
            for i, label in enumerate(labels):
                misses = np.array(df[label][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
                print((label+":").ljust(30) + str(np.mean(misses)).ljust(30) + "std: "+str(np.std(misses)))
                values[i].append(misses)
                ys[i].append(np.mean(misses))
                stds[i].append(np.std(misses))
    ylabel = "Number of misses"
    # single plot
    for i, label in enumerate(labels):
        title="Mean number of "+label+" for every optimization levels and key sizes"
        out_filename = "barplots/cache_misses_total_number_"+label+".png"
        barplot_single(xs, ys[i], stds[i], label, ylabel, title, out_filename)
        title="Number of "+label+" for every optimization levels and key sizes"
        out_filename = "boxplots/cache_misses_total_number_"+label+".png"
        boxplot_single(xs, values[i], ylabel, title, out_filename)
    # multi plot
    title = "Mean number of misses for Perf metrics for every optimization levels and key sizes"
    barplot_multiple_bars(xs, ys, stds, labels, ylabel, title, "cache_misses_total_number.png")


    # Total number of loads
    print("\n\n\n##### Loads #####")
    labels = ('cache-references','dTLB-loads','dTLB-stores','iTLB-loads','L1-dcache-loads','LLC-loads','LLC-stores')
    xs = []
    ys = [[] for i in range(len(labels))]
    stds = [[] for i in range(len(labels))]
    values = [[] for i in range(len(labels))]
    for ksize in KSIZES:    
        for opti in OPTIS:
            print("\nMean number of loads for ksize="+str(ksize)+" , opti="+OPTI_NAMES[opti]+" : ")
            xs.append("Optim="+OPTI_NAMES[opti]+ ", " + "ksize="+str(ksize))
            for i, label in enumerate(labels):
                loads = np.array(df[label][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
                print((label+":").ljust(30) + str(np.mean(loads)).ljust(30) + "std: "+str(np.std(loads)))
                values[i].append(loads)
                ys[i].append(np.mean(loads))
                stds[i].append(np.std(loads))
    ylabel = "Number of loads"
    # single plot
    for i, label in enumerate(labels):
        title="Mean number of "+label+" for every optimization levels and key sizes"
        out_filename = "barplots/cache_loads_total_number_"+label+".png"
        barplot_single(xs, ys[i], stds[i], label, ylabel, title, out_filename)
        title="Number of "+label+" for every optimization levels and key sizes"
        out_filename = "boxplots/cache_loads_total_number_"+label+".png"
        boxplot_single(xs, values[i], ylabel, title, out_filename)
    # multi plot
    title = "Mean number of loads for Perf metrics for every optimization levels and key sizes"
    barplot_multiple_bars(xs, ys, stds, labels, ylabel, title, "cache_loads_total_number.png")


