import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sn
import os


KSIZES = (8, 128)
OPTIS = (0, 1, 2, 3)
FILENAME_PERF = "data/perf_measurements.csv"
FILENAMES_RTIME = ["data/rtime_key_"+str(ksize)+"_opti_"+str(opti)+".csv" for ksize in KSIZES for opti in OPTIS]
PLOTS_DIRECTORY = "plots_phase2"
OPTI_NAMES = ("None", "Line by line", "Unroll", "Both")

def barplot2k(xs: list, ys: list, stds: list, labels: list, x_axis_name: str, y_axis_name: str, title: str, filename: str):
    fig, ax = plt.subplots(figsize=(10, 7))
    width = 0.2
    bars = []
    x = np.arange(len(xs))
    bars.append(ax.bar(x - width/2, ys[0], width, label=labels[0]))
    bars.append(ax.bar(x + width/2, ys[1], width, label=labels[1]))
    ax.errorbar(x - width/2, ys[0], yerr=stds[0], fmt="|", color="0")
    ax.errorbar(x + width/2, ys[1], yerr=stds[1], fmt="|", color="0")
    

    ax.set_xlabel(x_axis_name)
    ax.set_ylabel(y_axis_name)
    ax.legend(loc='upper left')
    
    ax.set_title(title)
    ax.set_xticks(x, labels=xs)
    ax.set_axisbelow(True)
    plt.xticks(rotation=80)
    plt.grid(axis='y', linestyle='dashed')
    fig.tight_layout()
    plt.savefig(filename+".pdf")
    plt.close()

def barplot_single_factor(xs: list, ys: list, stds: list, label: str, x_axis_name: str, y_axis_name: str, title: str, filename: str):
    fig  = plt.figure()
    plt.bar(xs, ys, label=label)
    plt.errorbar(xs, ys, yerr=stds, fmt="|", color="0")
    plt.xlabel(x_axis_name)
    plt.ylabel(y_axis_name)
    plt.title(title)
    plt.legend(loc="lower right")
    plt.grid(axis='y', linestyle='dashed')
    plt.rc('axes', axisbelow=True)
    plt.savefig(filename+".pdf")
    plt.close()

def plot_single_factor(xs: list, ys: list, stds: list, label: str, x_axis_name: str, y_axis_name: str, title: str, filename: str):
    fig  = plt.figure()
    plt.plot(xs, ys, label=label)
    plt.errorbar(xs, ys, yerr=stds, fmt="|", color="0")
    plt.xlabel(x_axis_name)
    plt.ylabel(y_axis_name)
    plt.ylim(ymin=0)
    plt.xlim(xmin=0)
    plt.title(title)
    plt.legend(loc="lower right")

    plt.grid(axis='y', linestyle='dashed')
    plt.rc('axes', axisbelow=True)
    plt.savefig(filename+".pdf")
    plt.close()


def get_values_for_2K(df, xs, ys, stds, response_variable):
    for fsize in df["fsize"].unique():
        for ksize in df['ksize'].unique():
            for request_rate in df['request_rate'].unique():

                xs.append("fsize="+str(fsize)+", ksize="+str(ksize)+", req_rate="+str(request_rate))

                for thread in df['thread'].unique():
                    tmp_df = df.loc[(df["fsize"] == fsize) & (df["ksize"] == ksize) & (df["request_rate"] == request_rate) & (df["thread"] == thread) & df["std"]]
                    
                    mean = tmp_df[response_variable].mean()
                    std = tmp_df["std"].mean()
                    

                    if (thread == 2):                
                        ys[0].append(mean)
                        stds[0].append(std)
                    elif (thread == 4):
                        ys[1].append(mean)
                        stds[1].append(std)

def get_values_single_factor_throughput(df, ys, stds, varying_factor, fixed_factors):
    for val in df[varying_factor].unique():
        tmp_df = df.loc[(df[fixed_factors[0][0]] == fixed_factors[0][1]) & (df[fixed_factors[1][0]] == fixed_factors[1][1]) & (df[fixed_factors[2][0]] == fixed_factors[2][1]) & (df[varying_factor] == val)]
        mean = tmp_df["throughput"].mean()
        std = tmp_df["throughput"].std()
        ys.append(mean)
        stds.append(std)

def get_values_single_factor_time(df, ys, stds, varying_factor, fixed_factors):
    for val in df[varying_factor].unique():
        tmp_df = df.loc[(df[fixed_factors[0][0]] == fixed_factors[0][1]) & (df[fixed_factors[1][0]] == fixed_factors[1][1]) & (df[fixed_factors[2][0]] == fixed_factors[2][1]) & (df[varying_factor] == val) & df["std"]]
        mean = tmp_df["response_time"].mean()
        std = tmp_df["std"].mean()
        ys.append(mean)
        stds.append(std)


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
        fig.subplots_adjust(bottom=0.3)
        fig.savefig(PLOTS_DIRECTORY+"/"+out_filename)

def barplot_cache_misses(xs, ys, stds, labels, out_filename):
    fig, ax = plt.subplots(figsize=(10, 7))
    width = 0.1
    bars = []
    x = np.arange(len(xs))
    bars.append(ax.bar(x - 3*width, ys[0], width, label=labels[0]))
    bars.append(ax.bar(x - 2*width, ys[1], width, label=labels[1]))
    bars.append(ax.bar(x - width, ys[2], width, label=labels[2]))
    bars.append(ax.bar(x, ys[3], width, label=labels[3]))
    bars.append(ax.bar(x + width, ys[4], width, label=labels[4]))
    bars.append(ax.bar(x + 2*width, ys[5], width, label=labels[5]))
    bars.append(ax.bar(x + 3*width, ys[6], width, label=labels[6]))
    ax.errorbar(x - 3*width, ys[0], yerr=stds[0], fmt="|", color="0")
    ax.errorbar(x - 2*width, ys[1], yerr=stds[1], fmt="|", color="0")
    ax.errorbar(x - width, ys[2], yerr=stds[2], fmt="|", color="0")
    ax.errorbar(x, ys[3], yerr=stds[3], fmt="|", color="0")
    ax.errorbar(x + width, ys[4], yerr=stds[4], fmt="|", color="0")
    ax.errorbar(x + 2*width, ys[5], yerr=stds[5], fmt="|", color="0")
    ax.errorbar(x + 3*width, ys[6], yerr=stds[6], fmt="|", color="0")

    #ax.set_xlabel("Perf metrics")
    ax.set_ylabel("Misses (%)")
    ax.legend(loc='upper right')
    
    ax.set_title("Mean percentage of misses for each metric for every optimization levels")
    ax.set_xticks(x, labels=xs)
    ax.set_axisbelow(True)
    plt.xticks(rotation=80)
    plt.grid(axis='y', linestyle='dashed')
    fig.tight_layout()
    plt.savefig(PLOTS_DIRECTORY+"/"+out_filename)
    plt.close()


    

if __name__ == "__main__":
    if not os.path.exists(PLOTS_DIRECTORY):
        os.makedirs(PLOTS_DIRECTORY)

    ## Response times ##
    data_8 = []
    data_128 = []
    labels_8 = []
    labels_128 = []
    for filename in FILENAMES_RTIME:
        df = pd.read_csv(filename)
        print(df.describe())
        if (df.iloc[0]["ksize"] == 128):
            labels_128.append("Optim="+OPTI_NAMES[int(df.iloc[0]["opti"])]+ ", " + "ksize="+str(df.iloc[0]["ksize"]))
            data_128.append(np.array(df["rtime"].tolist()) / 1000)
        else:
            labels_8.append("Optim="+OPTI_NAMES[int(df.iloc[0]["opti"])]+ ", " + "ksize="+str(df.iloc[0]["ksize"]))
            data_8.append(np.array(df["rtime"].tolist()) / 1000)
    boxplot_rtime(data_8, data_128, labels_8, labels_128, "2k_rtime_plot_split.pdf")
    boxplot_rtime(data_8, data_128, labels_8, labels_128, "2k_rtime_plot_single.pdf", "single")

    ## Perf measurements ##
    df = pd.read_csv(FILENAME_PERF)

    nb_metrics = 7
    xs = []
    ys = [[] for i in range(nb_metrics)]
    stds = [[] for i in range(nb_metrics)]
    labels = ('cache-misses', 'dTLB-load-misses', 'dTLB-store-misses', 'iTLB-load-misses', 'L1-dcache-load-misses', 'LLC-load-misses', 'LLC-store-misses')
    for ksize in KSIZES:    
        for opti in OPTIS:
            xs.append("Optim="+OPTI_NAMES[int(df.iloc[0]["opti"])]+ ", " + "ksize="+str(df.iloc[0]["ksize"]))
            # cache misses
            loads = np.array(df["cache-references"][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            misses = np.array(df['cache-misses'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            percentage = (np.array(misses) / loads) * 100
            ys[0].append(np.mean(percentage))
            stds[0].append(np.std(percentage))
            # dTLB-load-misses
            loads = np.array(df['dTLB-loads'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            misses = np.array(df['dTLB-load-misses'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            percentage = (np.array(misses) / loads) * 100
            ys[1].append(np.mean(percentage))
            stds[1].append(np.std(percentage))
            # dTLB-store-misses
            loads = np.array(df['dTLB-stores'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            misses = np.array(df['dTLB-store-misses'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            percentage = (np.array(misses) / loads) * 100
            ys[2].append(np.mean(percentage))
            stds[2].append(np.std(percentage))
            # iTLB-load-misses
            loads = np.array(df['iTLB-loads'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            misses = np.array(df['iTLB-load-misses'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            percentage = (np.array(misses) / loads) * 100
            ys[3].append(np.mean(percentage))
            stds[3].append(np.std(percentage))
            # L1-dcache-load-misses
            loads = np.array(df['L1-dcache-loads'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            misses = np.array(df['L1-dcache-load-misses'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            percentage = (np.array(misses) / loads) * 100
            ys[4].append(np.mean(percentage))
            stds[4].append(np.std(percentage))
            # LLC-load-misses
            loads = np.array(df['LLC-loads'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            misses = np.array(df['LLC-load-misses'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            percentage = (np.array(misses) / loads) * 100
            ys[5].append(np.mean(percentage))
            stds[5].append(np.std(percentage))
            # LLC-store-misses
            loads = np.array(df['LLC-stores'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            misses = np.array(df['LLC-store-misses'][(df["opti"] == opti) & (df["ksize"] == ksize)].tolist())
            percentage = (np.array(misses) / loads) * 100
            ys[6].append(np.mean(percentage))
            stds[6].append(np.std(percentage))
    
    barplot_cache_misses(xs, ys, stds, labels, "cache_misses_percentage.pdf")


    # TODO: graph with total number of miss / loads
        
    """
    corr_throughput = df_throughput.corr()
    corr_res_time = df_res_time.corr()
    print(corr_throughput.head())
    print(corr_res_time.head())
    fig, ax = plt.subplots(figsize=(3, 6))
    sn.heatmap(corr_throughput[["throughput"]], annot=True)
    plt.margins(5)
    plt.savefig("data/throughput_correlation.pdf")
    plt.close()
    fig, ax = plt.subplots(figsize=(3, 6))
    sn.heatmap(corr_res_time[["response_time"]], annot=True, linewidths=0.3)
    plt.margins(5)
    plt.savefig("data/response_time_correlation.pdf")
    plt.close()
    """


                
    