import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sn
from sklearn.tree import DecisionTreeClassifier

def barplot_2K(xs: list, ys: list, stds: list, labels: list, x_axis_name: str, y_axis_name: str, title: str, filename: str):
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

def barplot_single_factor(xs: list, ys: list, stds: list, x_axis_name: str, y_axis_name: str, title: str, filename: str):
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
                    

                    if (fsize == 128):                
                        ys[0].append(mean)
                        stds[0].append(std)
                    elif (fsize == 256):
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
    FILENAME_THROUGHPUT = "data/2k_throughput_easy.csv"
    FILENAME_RESPONSE_TIME = "data/2k_response_time_easy.csv"
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
    labels = ("fsize 128", "fsize 256")
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
    barplot_2K(xs, ys, stds, labels, x_axis_name, y_axis_name, title, filename)


    ### Mean response time versus number of threads
    labels = ("fsize 128", "fsize 256")
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
    barplot_2K(xs, ys, stds, labels, x_axis_name, y_axis_name, title, filename)


    """
    ### Another plot
    xs = [1,2,4,8] # xs, ys and stds, same dimension
    ys = []
    stds = []
    get_values_single_factor(df, ys, stds, "threads", [("fsize",512),("ksize",256),("request_rate",)], response_variable)
    x_axis_name = "Threads"
    y_axis_name = ""
    title = ""
    filename = "data/"
    barplot_single_factor(xs, ys, stds, x_axis_name, y_axis_name, title, filename)
    """

    