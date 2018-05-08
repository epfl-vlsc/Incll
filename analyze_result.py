import json
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

sns.set_style("whitegrid")


def readFile(file):
    exp_names = []
    tids = []
    ops_list = []

    with open("dump/"+file) as f:
        notebook = json.load(f)
        data = notebook["data"]

        for experiment in data:
            for e in data[experiment]:
                if "removepos" in e: continue

                ops_per_second = float(e["ops_per_sec"])
                tid = int(e["thread"])
                exp_name = e["test"]

                tids.append(tid)
                ops_list.append(ops_per_second)
                exp_names.append(exp_name)

    d = {
        "tids":tids,
        "exp_names":exp_names,
        "ops_list":ops_list
    }

    return pd.DataFrame(d)


def plot_df(df, file, name):
    ax = sns.barplot(
        x="exp_names", y="ops_list", hue="tids", data=df)

    plt.title(name)
    plt.xlabel("Thread ids")
    plt.ylabel("Ops per second")
    plt.savefig("dump/"+file[:-4]+"png")


if __name__ == '__main__':
    titles = ["Global Flush", "No Global Flush"]

    files = [
        "notebook_intensive_globalflush.json",
        "notebook_intensive_noglobalflush.json"
    ]

    for file, title in zip(files, titles):
        df = readFile(file)
        print(df.head())
        plot_df(df, file, title)
