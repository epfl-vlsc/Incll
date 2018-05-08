import json
import pandas as pd


def readFile():
    file = "notebook-mttest.json"
    with open(file) as f:
        notebook = json.load(f)
        data = notebook["data"]

        for experiment in data:
            for e in data[experiment]:
                if "removepos" in e: continue

                ops_per_second = float(e["ops_per_sec"])
                tid = int(e["thread"])


if __name__ == '__main__':
    readFile()