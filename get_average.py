import json


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

    return sum(ops_list) / len(ops_list)


if __name__ == '__main__':
    avg = get_avg_ops()
    print(avg)
