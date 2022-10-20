import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sn


def barplot2k(xs, ys, stds, labels, x_axis_name, y_axis_name, title, filename):
    # type: (list, list, list, list, str, str, str, str) -> None
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
    ax.legend()
    
    ax.set_title(title)
    ax.set_xticks(x, labels=xs)
    plt.xticks(rotation=80)
    plt.grid(axis='y')
    fig.tight_layout()
    plt.savefig(filename+".pdf")
    plt.close()

def barplot_single_factor(xs, ys, stds, x_axis_name, y_axis_name, title, filename):
    # type: (list, list, list, str, str, str, str) -> None
    fig  = plt.figure()
    plt.bar(xs, ys)
    plt.errorbar(xs, ys, yerr=stds, fmt="|", color="0")
    plt.legend()
    plt.xlabel(x_axis_name)
    plt.ylabel(y_axis_name)
    plt.title(title)
    plt.savefig(filename+".pdf")
    plt.close()

def get_values_for_2K(df, xs, ys, stds, response_variable):
    for thread in df["thread"].unique():
        for ksize in df['ksize'].unique():
            for request_rate in df['request_rate'].unique():

                xs.append("threads="+str(thread)+", ksize="+str(ksize)+", req_rate="+str(request_rate))

                for fsize in df['fsize'].unique():
                    tmp_df = df.loc[(df["fsize"] == fsize) & (df["ksize"] == ksize) & (df["request_rate"] == request_rate) & (df["thread"] == thread)]
                    
                    mean = tmp_df[response_variable].mean()
                    std = tmp_df[response_variable].std()
                    

                    if (fsize == 256):                
                        ys[0].append(mean)
                        stds[0].append(std)
                    elif (fsize == 512):
                        ys[1].append(mean)
                        stds[1].append(std)

def get_values_single_factor(df, ys, stds, varying_factor, fixed_factors, response_variable):
    for val in df[varying_factor].unique():
        tmp_df = df.loc[(df[fixed_factors[0][0]] == fixed_factors[0][1]) & (df[fixed_factors[1][0]] == fixed_factors[1][1]) & (df[fixed_factors[2][0]] == fixed_factors[2][1]) & (df[varying_factor] == val)]
        mean = tmp_df[response_variable].mean()
        std = tmp_df[response_variable].std()
        ys.append(mean)
        stds.append(std)


    

if __name__ == "__main__":
    FILENAME_THROUGHPUT = "data/2k_throughput_hard.csv"
    FILENAME_RESPONSE_TIME = "data/2k_response_time_hard.csv"
    df_throughput = pd.read_csv(FILENAME_THROUGHPUT)
    df_res_time = pd.read_csv(FILENAME_RESPONSE_TIME)
    corr_throughput = df_throughput.corr()
    corr_res_time = df_res_time.corr()
    print(corr_throughput.head())
    print(corr_res_time.head())
    sn.heatmap(corr_throughput[["throughput"]], annot=True)
    plt.margins(5)
    plt.savefig("data/throughput_correlation.pdf")
    plt.close()
    sn.heatmap(corr_res_time[["response_time"]], annot=True)
    plt.margins(5)
    plt.savefig("data/response_time_correlation.pdf")
    plt.close()


                
    ### Throughput versus number of threads
    labels = ("fsize 256", "fsize 512")
    xs = []
    ys = [[], []]
    stds = [[], []]
    get_values_for_2K(df_throughput, xs, ys, stds, "throughput")
    #reverse array
    #xs = xs[::-1]
    #ys[:] = ys[:][::-1]
    #stds[:] = stds[:][::-1]
    
    x_axis_name = "Parameters"
    y_axis_name = "Throughput [requests/s]"
    title = "Throughput versus number of threads"
    filename = "data/throughput_vs_threads"
    barplot2k(xs, ys, stds, labels, x_axis_name, y_axis_name, title, filename)


    ### Mean response time versus number of threads
    labels = ("fsize 256", "fsize 512")
    xs = []
    ys = [[], []]
    stds = [[], []]
    get_values_for_2K(df_res_time, xs, ys, stds, "response_time")
    #reverse array
    #xs = xs[::-1]
    #ys[:] = ys[:][::-1]
    #stds[:] = stds[:][::-1]
    
    x_axis_name = "Parameters"
    y_axis_name = "Mean response time [ms]"
    title = "Mean response time versus number of threads"
    filename = "data/mean_response_time_vs_threads"
    barplot2k(xs, ys, stds, labels, x_axis_name, y_axis_name, title, filename)


    
    ### Threads variations
    xs = [1,2,4,8] # xs, ys and stds, same dimension
    FILENAME_THROUGHPUT = "data/thread_throughput.csv"
    FILENAME_RESPONSE_TIME = "data/thread_response_time.csv"
    df_thread_throughput = pd.read_csv(FILENAME_THROUGHPUT)
    df_thread_res_time = pd.read_csv(FILENAME_RESPONSE_TIME)
    ys = []
    stds = []
    get_values_single_factor(df_thread_throughput, ys, stds, "threads", [("fsize",512),("ksize",256),("request_rate",100)], "throughput")
    x_axis_name = "Threads"
    y_axis_name = "Throughput [requests/s]"
    title = "Throughput in terms of threads"
    filename = "data/thread_throughput"
    barplot_single_factor(xs, ys, stds, x_axis_name, y_axis_name, title, filename)

    ys = []
    stds = []
    get_values_single_factor(df_thread_res_time, ys, stds, "threads", [("fsize",512),("ksize",256),("request_rate",100)], "response_time")
    y_axis_name = "Mean response time [ms]"
    title = "Response time in terms of threads"
    filename = "data/thread_res_time"
    barplot_single_factor(xs, ys, stds, x_axis_name, y_axis_name, title, filename)
    

    ### FSize variations 1

    ### FSize variations 2

    ### KSize variations

    ### Requests variations
    

    