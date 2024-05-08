import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys

def plot(df: pd.DataFrame, output_path: str):
    # Create array of colors and hatches
    colors = ['#4d759a', '#f35a1c', '#238b8c']
    hatches = ['///', '..', 'xx']

    # Set the figure size
    plt.figure(figsize=(10, 8))

    # X-axis labels (benchmark names)
    num_benchmarks = len(set(df['benchmark']))
    x = np.arange(num_benchmarks)
    plt.xticks(x, df['benchmark'].drop_duplicates(), rotation=315, horizontalalignment='left'
               , fontsize='14')

    # Bar width
    bar_width:float = 0.25

    # Get number of implementations
    num_impl:int = len(df['implementation'].unique())
    x = x - bar_width * (num_impl - 1) / 2

    for i, implementation in enumerate(df['implementation'].unique()):
        # Filter the dataframe by implementation
        df_impl = df[df['implementation'] == implementation]

        # Plot bars for the implementation
        plt.bar(x, df_impl['median-time'], width=bar_width, label=implementation, color=colors[i], 
                yerr=df_impl['stdev'], ecolor='black', capsize=3, error_kw={'elinewidth': 2}, hatch=hatches[i])

        # Increment x by bar_width
        x = x + bar_width

    # set y-axis to log scale
    plt.yscale('log')

    plt.yticks(fontsize='12')

    # Add labels and title
    plt.ylabel('Time (s)', fontsize='16')
    plt.legend(fontsize='16', loc = 'upper right', ncols=1)

    # Show the plot
    plt.tight_layout()
    plt.savefig(output_path)

if __name__ == "__main__":
    # Check first argument is the output file
    if len(sys.argv) != 2:
        print("Usage: python plot.py <output.png>")
        sys.exit(1)

    output_path:str = sys.argv[1]

    # Load the data
    df1 = pd.read_csv('../data/raw/1100max_l0.csv')
    df2 = pd.read_csv('../data/raw/1100max_ocl.csv')
    df3 = pd.read_csv('../data/raw/1100max_omp.csv')

    # Add a column to each dataframe to indicate the implementation
    df1['implementation'] = 'SYCL (Level0)'
    df2['implementation'] = 'SYCL (OpenCL)'
    df3['implementation'] = 'OpenMP'
    
    # Set order column by implementation
    df1['order'] = 1
    df2['order'] = 2
    df3['order'] = 3

    # Merge the data
    df = pd.concat([df1, df2, df3])

    # Remove benchmark implementation suffix from the dataframes
    df['benchmark'] = df['benchmark'].str.replace('-sycl', '')
    df['benchmark'] = df['benchmark'].str.replace('-omp', '')

    # Remove the 'repetitions' and 'times' column
    df = df.drop(columns=['repetitions', 'times'])

    # Sort the data by benchmark and order
    df = df.sort_values(by=['benchmark', 'order'])

    # Plot the data
    plot(df, output_path)