import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sn
from sklearn.tree import DecisionTreeClassifier

def barplot_2K(xs: list, ys: list, stds: list, labels: list, label: str, x_axis_name: str, y_axis_name: str, title: str, filename: str):
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
    fig.tight_layout()
    plt.savefig(filename+".pdf")
    plt.close()

def get_values_for_2K(df, xs, ys, stds, response_variable):
        for fsize in df["fsize"].unique():
            for ksize in df['ksize'].unique():
                for request_rate in df['request_rate'].unique():

                    print("thread values : ", df['thread'].unique())
                    xs.append("fsize="+str(fsize)+", ksize="+str(ksize)+", req_rate="+str(request_rate))

                    for thread in df['thread'].unique():
                        tmp_df = df.loc[(df["fsize"] == fsize) & (df["ksize"] == ksize) & (df["request_rate"] == request_rate) & (df["thread"] == thread)]
                        
                        mean = tmp_df[response_variable].mean()
                        std = tmp_df[response_variable].std()
                        print(mean)
                        print(std)

                        if (thread == 2):                
                            ys[0].append(mean)
                            stds[0].append(std)
                        elif (thread == 4):
                            ys[1].append(mean)
                            stds[1].append(std)


    

if __name__ == "__main__":
    FILENAME_THROUGHPUT = "data/20_10_2022_12_02_24_throughput.csv"
    FILENAME_RESPONSE_TIME = "data/20_10_2022_12_02_24_response_time.csv"
    df_throughput = pd.read_csv(FILENAME_THROUGHPUT)
    df_res_time = pd.read_csv(FILENAME_RESPONSE_TIME)
    print(df_throughput.dtypes)
    corr_throughput = df_throughput.corr()
    corr_res_time = df_res_time.corr()
    sn.heatmap(corr_throughput, annot=True)
    plt.margins(5)
    plt.savefig("data/throughput_correlation.pdf")
    plt.close()
    sn.heatmap(corr_res_time, annot=True)
    plt.margins(5)
    plt.savefig("data/response_time_correlation.pdf")
    plt.close()


                
    ### Throughput versus number of threads
    labels = ("2 threads", "4 threads")
    xs = []
    ys = [[], []]
    stds = [[], []]
    get_values_for_2K(df_throughput, xs, ys, stds, "throughput")
    label = "Threads"
    x_axis_name = "Parameters"
    y_axis_name = "Throughput"
    title = "Throughput versus number of threads"
    filename = "data/throughput_vs_threads"
    barplot_2K(xs, ys, stds, labels, label, x_axis_name, y_axis_name, title, filename)


    ### Mean response time versus number of threads
    labels = ("2 threads", "4 threads")
    xs = []
    ys = [[], []]
    stds = [[], []]
    get_values_for_2K(df_res_time, xs, ys, stds, "response_time")
    label = "Threads"
    x_axis_name = "Parameters"
    y_axis_name = "Mean response time"
    title = "Mean response time versus number of threads"
    filename = "data/mean_response_time_vs_threads"
    barplot_2K(xs, ys, stds, labels, label, x_axis_name, y_axis_name, title, filename)

    


    ### Another plot

    