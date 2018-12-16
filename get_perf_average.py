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

    def get_max(self):
        return max(self.values)


class Stats:
    def __init__(self, name):
        self.trials = []
        self.name = name

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

    def get_max(self):
        return np.mean([t.get_max() for t in self.trials])
       
    def get_name(self):
        return self.name

counter_names = [
    "instructions", "cycles", 
    "l1dc_loadreferences", "l1dc_loadmisses", 
    "llc_loadreferences","llc_loadmisses"
]

stat_names = [
    "Total"
]

def get_stats(data):
    counters_stats = [Stats(counter_name) for i,counter_name in enumerate(counter_names)]

    for i, run in enumerate(data):
        for thread_run in data[run]:
            for counter_stats in counters_stats:            
                counter_result = thread_run[counter_stats.get_name()]            
                counter_stats.add(i, counter_result)

    return counters_stats


def analyze_ops_stats(workload_name):
    data = get_notebook()
    counters_stats = get_stats(data)
    
    s = []    
    for counter_stats in counters_stats:
        counter_sum = counter_stats.get_sum()
        s.extend([counter_sum])
    """
    op_stat_sum = op_stats.get_sum()
    op_stat_avg = op_stats.get_avg()
    op_stat_sum_std = op_stats.get_sum_std()
    op_stat_stdp = op_stat_sum_std / op_stat_sum
	"""
    s = [str(e) for e in s]
    print(",".join(s) + "," + workload_name)


def print_header():
    s = []
    for counter_name in counter_names:
        s.extend([stat_name + "_" +counter_name for stat_name in stat_names])
    print(",".join(s) + ",Workload")
        

if __name__ == '__main__':
    file_or_workload = sys.argv[1] if len(sys.argv) > 1 else None    
    if file_or_workload.endswith(".txt"):
        print_header()
    else:
        analyze_ops_stats(file_or_workload)

