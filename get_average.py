import json
import sys


def get_avg_ops():
    ops_list = []

    text = "notebook-mttest.json"
    with open(text, 'r') as f:
        json_obj = json.load(f)
        data = json_obj["data"]
        for run in data:
            for thread_run in data[run]:
                ops = thread_run["ops"]
                ops_list.append(ops)

    d = {
        "sumops": sum(ops_list),
        "avgops": sum(ops_list) / len(ops_list)
    }
    return d


if __name__ == '__main__':
    avg = get_avg_ops()
    if len(sys.argv) > 1:
        print_type = sys.argv[1]
        print(avg[print_type])
    else:
        print(avg)
