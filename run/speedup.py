import pandas as pd
import numpy as np
import sys

if __name__ == "__main__":
    # Check two arguments are provided both are csv files
    if len(sys.argv) != 3:
        print("Usage: python speedup.py file1.csv file2.csv")
        sys.exit(1)

    file1:str = sys.argv[1]
    file2:str = sys.argv[2]

    # Load the data
    df1 = pd.read_csv(file1)
    df2 = pd.read_csv(file2)

    # Sort the data by benchmark
    df1 = df1.sort_values(by=['benchmark'])
    df2 = df2.sort_values(by=['benchmark'])

    # clean zeros
    zeros = lambda x, y: (x, y) if y != 0 and x != 0 else -1

    div = lambda x, y: x / y if y != 0 and x != 0 else -1
    median1 = df1['median-time'].to_list()
    median2 = df2['median-time'].to_list()

    # use lamda function to calculate the speedup
    speedup = list(map(div, median1, median2))

    # Use filter to remove the -1 values
    speedup = list(filter(lambda x: x != -1, speedup))

    # Calculate the mean speedup, and std deviation
    mean_speedup = np.mean(speedup)
    std_speedup = np.std(speedup)

    print(f"Mean speedup: {mean_speedup:.2f}")
    print(f"Std deviation: {std_speedup:.2f}")