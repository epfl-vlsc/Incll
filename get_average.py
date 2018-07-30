import json
import sys
import numpy as np

def get_notebook():
    text = "notebook-mttest.json"
    with open(text, 'r') as f:
        json_obj = json.load(f)
        data = json_obj["data"]
        return data


class Trial:
    def __init__(self):
        self.values = []

    def add_val(self, val):
        self.values.append(val)

    def get_avg(self):
        return np.mean(self.values)

    def get_sum(self):
        return sum(self.values)


class OpStats:
    def __init__(self):
        self.trials = []

    def add(self, i, val):
        if len(self.trials) == i:
            self.trials.append(Trial())
        self.trials[i].add_val(val)

    def get_sum(self):
        return np.mean([t.get_sum() for t in self.trials])

    def get_sum_std(self):
        return np.std([t.get_sum() for t in self.trials])

    def get_avg(self):
        return np.mean([t.get_avg() for t in self.trials])


def get_ops_stats(data):
    op_stats = OpStats()

    for i, run in enumerate(data):
        for thread_run in data[run]:
            ops = thread_run["ops"]
            op_stats.add(i, ops)

    return op_stats


def analyze_ops_stats():
    params = sys.argv[1] if len(sys.argv) > 1 else None
    sf = "{},{},{},{}" if params else "{},{},{}"

    data = get_notebook()
    op_stats = get_ops_stats(data)

    s = sf.format(op_stats.get_sum(), op_stats.get_avg(), op_stats.get_sum_std(), params)
    print(s)


if __name__ == '__main__':
    analyze_ops_stats()

