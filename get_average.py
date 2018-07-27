import json
import sys

"""
ops_types = [
    "ops",
    "get_ops",
    "put_ops",
    "rem_ops",
    "scan_ops"
]
"""

ops_types = [
    "ops"
]


def get_notebook():
    text = "notebook-mttest.json"
    with open(text, 'r') as f:
        json_obj = json.load(f)
        data = json_obj["data"]
        return data


def get_ops_stats(data):
    op_stats = {op_type: [] for op_type in ops_types}

    for run in data:
        for thread_run in data[run]:
            for op_type in ops_types:
                if op_type in thread_run:
                    ops = thread_run[op_type]
                    op_stats[op_type].append(ops)

    return op_stats


def analyze_ops_stats():
    params = sys.argv[1] if len(sys.argv) > 1 else None
    sf = "{},{},{}" if params else "{},{}"

    data = get_notebook()
    op_stats = get_ops_stats(data)
    for k, v in op_stats.iteritems():
        s = sf.format(sum(v), sum(v)/len(v), params)
        print(s)


if __name__ == '__main__':
    analyze_ops_stats()

