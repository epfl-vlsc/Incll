"""
/* Modification of Masstree
 * VLSC Laboratory
 * Copyright (c) 2018-2019 Ecole Polytechnique Federale de Lausanne
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Masstree LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Masstree LICENSE file; the license in that file
 * is legally binding.
 */
"""

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

    def get_max(self):
        return np.max(self.values)


class OpStats:
    def __init__(self):
        self.trials = []

    def add(self, i, val):
        if len(self.trials) == i:
            self.trials.append(Trial())
        self.trials[i].add_val(val)

    def get_avg(self):
        return np.mean([t.get_max() for t in self.trials])

    def get_std(self):
        return np.std([t.get_max() for t in self.trials])


def get_ops_stats(data):
    op_stats = OpStats()

    for i, run in enumerate(data):
        for thread_run in data[run]:
            ops = thread_run["recovery_time"]
            op_stats.add(i, ops)

    return op_stats


def analyze_ops_stats():
    params = sys.argv[1] if len(sys.argv) > 1 else None
    sf = "{},{},{},{}" if params else "{},{},{}"

    data = get_notebook()
    op_stats = get_ops_stats(data)

    stat_avg = op_stats.get_avg()
    stat_std = op_stats.get_std()
    stat_stdp = stat_std / stat_avg

    s = sf.format(stat_avg, stat_std, stat_stdp, params)
    print(s)


if __name__ == '__main__':
    analyze_ops_stats()

