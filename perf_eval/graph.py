import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sn


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
    plt.legend()
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
    plt.title(title)
    plt.legend()
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
                    tmp_df = df.loc[(df["fsize"] == fsize) & (df["ksize"] == ksize) & (df["request_rate"] == request_rate) & (df["thread"] == thread)]
                    
                    mean = tmp_df[response_variable].mean()
                    std = tmp_df["std"].mean()
                    

                    if (thread == 2):                
                        ys[0].append(mean)
                        stds[0].append(std)
                    elif (thread == 4):
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
    FILENAME_RESPONSE_TIME = "data/2k_response_time_easy.csv"
    df_throughput = pd.read_csv(FILENAME_THROUGHPUT)
    df_res_time = pd.read_csv(FILENAME_RESPONSE_TIME)
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


                
    ### Throughput versus number of threads
    labels = ("2 threads", "4 threads")
    xs = []
    ys = [[], []]
    stds = [[], []]
    get_values_for_2K(df_throughput, xs, ys, stds, "throughput")
    #reverse array
    xs = xs[::-1]
    ys[0] = ys[0][::-1]
    stds[0] = stds[0][::-1]
    ys[1] = ys[1][::-1]
    stds[1] = stds[1][::-1]
    
    x_axis_name = "Parameters"
    y_axis_name = "Throughput [requests/s]"
    title = "Throughput versus number of threads"
    filename = "data/throughput_vs_threads"
    barplot2k(xs, ys, stds, labels, x_axis_name, y_axis_name, title, filename)


    ### Mean response time versus number of threads
    labels = ("2 threads", "4 threads")
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
    xs = ["1","2","4","8"] # xs, ys and stds, same dimension
    FILENAME_THROUGHPUT = "data/thread_throughput.csv"
    FILENAME_RESPONSE_TIME = "data/thread_response_time.csv"
    df_thread_throughput = pd.read_csv(FILENAME_THROUGHPUT)
    df_thread_res_time = pd.read_csv(FILENAME_RESPONSE_TIME)
    ys = []
    stds = []
    label="file_size=512, key_size=256, request_rate=100"
    get_values_single_factor(df_thread_throughput, ys, stds, "thread", [("fsize",512),("ksize",256),("request_rate",100)], "throughput")
    x_axis_name = "Threads"
    y_axis_name = "Throughput [requests/s]"
    title = "Throughput in terms of threads"
    filename = "data/thread_throughput"
    barplot_single_factor(xs, ys, stds, label, x_axis_name, y_axis_name, title, filename)

    ys = []
    stds = []
    get_values_single_factor(df_thread_res_time, ys, stds, "thread", [("fsize",512),("ksize",256),("request_rate",100)], "response_time")
    y_axis_name = "Mean response time [ms]"
    title = "Response time in terms of threads"
    filename = "data/thread_res_time"
    barplot_single_factor(xs, ys, stds, label, x_axis_name, y_axis_name, title, filename)
    

    ### FSize variations
    xs = ["64","128","256","512"] # xs, ys and stds, same dimension
    FILENAME_THROUGHPUT = "data/fsize_queuing_throughput.csv"
    FILENAME_RESPONSE_TIME = "data/fsize_queuing_response_time.csv"
    df_file_throughput = pd.read_csv(FILENAME_THROUGHPUT)
    df_file_res_time = pd.read_csv(FILENAME_RESPONSE_TIME)
    ys = []
    stds = []
    label="key_size=32, request_rate=1000, thread=1"
    get_values_single_factor(df_file_throughput, ys, stds, "fsize", [("ksize",32),("request_rate",1000),("thread",1)], "throughput")
    x_axis_name = "file size [bytes]"
    y_axis_name = "Throughput [requests/s]"
    title = "Throughput in terms of file sizes"
    filename = "data/fsize_throughput"
    barplot_single_factor(xs, ys, stds, label, x_axis_name, y_axis_name, title, filename)

    ys = []
    stds = []
    get_values_single_factor(df_file_res_time, ys, stds, "fsize", [("ksize",32),("request_rate",1000),("thread",1)], "response_time")
    y_axis_name = "Mean response time [ms]"
    title = "Response time in terms of file sizes"
    filename = "data/fsize_res_time"
    barplot_single_factor(xs, ys, stds, label, x_axis_name, y_axis_name, title, filename)

    ### KSize variations

    xs = ["8","16","32","64","128"] # xs, ys and stds, same dimension
    FILENAME_THROUGHPUT = "data/ksize_queuing_throughput.csv"
    FILENAME_RESPONSE_TIME = "data/ksize_queuing_response_time.csv"
    df_key_throughput = pd.read_csv(FILENAME_THROUGHPUT)
    df_key_res_time = pd.read_csv(FILENAME_RESPONSE_TIME)
    ys = []
    stds = []
    label="file_size=256, request_rate=500, thread=1"
    get_values_single_factor(df_key_throughput, ys, stds, "ksize", [("fsize",256),("request_rate",500),("thread",1)], "throughput")
    x_axis_name = "key size [bytes]"
    y_axis_name = "Throughput [requests/s]"
    title = "Throughput in terms of key sizes"
    filename = "data/ksize_throughput"
    barplot_single_factor(xs, ys, stds, label, x_axis_name, y_axis_name, title, filename)

    ys = []
    stds = []
    get_values_single_factor(df_key_res_time, ys, stds, "ksize", [("fsize",256),("request_rate",500),("thread",1)], "response_time")
    y_axis_name = "Mean response time [ms]"
    title = "Response time in terms of key sizes"
    filename = "data/ksize_res_time"
    barplot_single_factor(xs, ys, stds, label, x_axis_name, y_axis_name, title, filename)

    ### Requests variations
    
    xs = ["200","400","600","800"] # xs, ys and stds, same dimension
    FILENAME_THROUGHPUT = "data/rate_throughput.csv"
    FILENAME_RESPONSE_TIME = "data/rate_response_time.csv"
    df_rate_throughput = pd.read_csv(FILENAME_THROUGHPUT)
    df_rate_res_time = pd.read_csv(FILENAME_RESPONSE_TIME)
    ys = []
    stds = []
    label="file_size=256, key_size=32, thread=1"
    get_values_single_factor(df_rate_throughput, ys, stds, "request_rate", [("fsize",256),("ksize",32),("thread",1)], "throughput")
    x_axis_name = "Mean request rate [requests/s]"
    y_axis_name = "Throughput [requests/s]"
    title = "Throughput in terms of mean request rates"
    filename = "data/rate_throughput"
    barplot_single_factor(xs, ys, stds, label, x_axis_name, y_axis_name, title, filename)

    ys = []
    stds = []
    get_values_single_factor(df_rate_res_time, ys, stds, "request_rate", [("fsize",256),("ksize",32),("thread",1)], "response_time")
    y_axis_name = "Mean response time [ms]"
    title = "Response time in terms of mean request rates"
    filename = "data/rate_res_time"
    barplot_single_factor(xs, ys, stds, label, x_axis_name, y_axis_name, title, filename)
    