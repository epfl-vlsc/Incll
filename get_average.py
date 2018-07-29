import json
import sys
import numpy as np

def get_notebook():
    text = "notebook-mttest.json"
    with open(text, 'r') as f:
        json_obj = json.load(f)
        data = json_obj["data"]
        return data


class OpStats:
    def __init__(self):
        self.n_trials = 0
        self.values = []

    def add(self, val):
        self.values.append(val)

    def inc_trial(self):
        self.n_trials += 1

    def get_avg(self):
        return np.mean(self.values)

    def get_std(self):
        return np.std(self.values)

    def get_sum(self):
        return sum(self.values) / self.n_trials


def get_ops_stats(data):
    op_stats = OpStats()

    for run in data:
        for thread_run in data[run]:
            ops = thread_run["ops"]
            op_stats.add(ops)
        op_stats.inc_trial()

    return op_stats


def analyze_ops_stats():
    params = sys.argv[1] if len(sys.argv) > 1 else None
    sf = "{},{},{},{}" if params else "{},{},{}"

    data = get_notebook()
    op_stats = get_ops_stats(data)

    s = sf.format(op_stats.get_sum(), op_stats.get_avg(), op_stats.get_std(), params)
    print(s)


if __name__ == '__main__':
    analyze_ops_stats()

