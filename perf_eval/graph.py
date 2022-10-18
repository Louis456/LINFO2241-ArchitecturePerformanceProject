import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

def plot(xs: list, ys: list, labels: list, x_axis_name: str, y_axis_name: str, title: str, filename: str):
    fig = plt.figure()
    plt.plot(xs[0], ys[0], label=labels[0]) # First line
    plt.plot(xs[1], ys[1], label=labels[1]) # Second line
    plt.plot(xs[2], ys[2], label=labels[2]) # Third line
    plt.xlabel(x_axis_name)
    plt.ylabel(y_axis_name)
    #plt.xticks(np.arange(0, 100, 30))
    #plt.yticks([-1.5, 0, 1.5])
    plt.title(title)
    plt.savefig(filename+".pdf")
    plt.close()

if __name__ == "__main__":
    FILENAME_THROUGHPUT = "data/18_10_2022_12_05_41_throughput.csv"
    FILENAME_RESPONSE_TIME = "data/18_10_2022_12_05_41_response_time.csv"
    df_throughput = pd.read_csv(FILENAME_THROUGHPUT)

    # TODO: plot(....)