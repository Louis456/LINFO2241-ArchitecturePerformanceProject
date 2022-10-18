import os
import numpy as np
import subprocess
import time
import re
import pandas as pd
from datetime import datetime
import graph


if __name__ == "__main__":

    FILENAME_THROUGHPUT = "data/18_10_2022_12_05_41_throughput.csv"
    FILENAME_RESPONSE_TIME = "data/18_10_2022_12_05_41_response_time.csv"
    df = pd.read_csv(FILENAME_THROUGHPUT)
    print(df.to_string())
    for val in df["fsize"].unique():
        print(val)
    