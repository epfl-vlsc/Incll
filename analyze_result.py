import json
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

sns.set_style("whitegrid")


def readFile(file, expNameDict):
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
                exp_names.append(expNameDict[exp_name])

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
    plt.show()
    #plt.savefig("dump/"+file[:-4]+"png")


def plot_comparison(df, overheads):
    names = overheads["exp_names"].as_matrix()
    ovhds = overheads["overhead"].as_matrix()

    xticks = [name+"\n"+"{0:.2f}".format(ovhd) for name, ovhd in zip(names, ovhds)]

    ax = sns.barplot(
        x="exp_names", y="ops_list", data=df)

    ax.set_xticklabels(xticks)

    plt.title("No Flush vs. Global Flush (with different frequencies)")
    plt.xlabel("Executions")
    plt.ylabel("Ops per second")
    plt.show()


def calculate_overheads(df):
    means = df.groupby(["exp_names"], as_index=False).mean()
    print(means.head())

    base = means[means["exp_names"] == "No Flush"]["ops_list"].as_matrix()
    comp = means["ops_list"].as_matrix()

    print(base, comp)

    means["overhead"] = (base - comp) * 100 / base
    return means


def plot_global_flush():
    expNameDict = {"intensive_gl_"+str(i):"GL Freq " + str(i) for i in range(5, 50)}
    expNameDict["intensive_"] = "No Flush"

    file = "global_flush.json"
    df = readFile(file, expNameDict)
    print(df.head())
    overheads = calculate_overheads(df)

    plot_comparison(df, overheads)


if __name__ == '__main__':
    plot_global_flush()
