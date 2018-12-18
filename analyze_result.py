import json
import sys
import pandas as pd

exp_col = "exp_no"
ops_col = "ops"
test_col = "test"
ignores = ["table", "trial", "mb"]
time_cols = ["thread_time", "workload_time"]
info_cols = ["delay_count", "keys", "threads", "num_operations"]
group_cols = info_cols + ["test"]
special_ops_cols = ["rem_ops", "scan_ops", "get_ops", "put_ops"]
misc_cols = ["thread", exp_col]
general_drop = info_cols + special_ops_cols + misc_cols
indiv_group_cols = group_cols + [exp_col]
write_to_file = False
filename = None

def write_file(df, ext):  
    if write_to_file and filename:    
        fname = filename + ext
        print("Write to " + fname)
        df.to_csv(fname, index=False)

def get_notebook():
    text = "notebook-mttest.json"
    with open(text, 'r') as f:
        json_obj = json.load(f)
        data = json_obj["data"]
        return data

def learn_headers(data):
    data_dict = dict()
    for i, run in enumerate(data):
        for thread_dict in data[run]:
            for tkey, tval in thread_dict.iteritems():
                if tkey in ignores:
                    continue
                
                data_dict[tkey] = []
            
            data_dict[exp_col] = []
            return data_dict

def create_raw_table(data):
    data_dict = learn_headers(data)

    for i, run in enumerate(data):
        for thread_dict in data[run]:
            data_dict[exp_col].append(i)
            for tkey, tval in thread_dict.iteritems():
                if tkey in ignores:
                    continue
                
                if tkey not in data_dict:
                    raise ValueError('Different header')
                
                data_dict[tkey].append(tval)
    
    df = pd.DataFrame(data_dict)  
    write_file(df, "_raw.csv")   
    return df

def create_summary_table(data_table, summary_col):
    res = data_table.groupby(indiv_group_cols).sum()
    res = res.reset_index()
    res = res.groupby(group_cols)
    res = res[summary_col].describe()
    res = res.reset_index()
    res = res.drop(info_cols, axis=1)
    write_file(res, "_" + summary_col + ".csv")  

def create_general_summary(data_table):
    res = data_table.groupby(indiv_group_cols).sum()
    res = res.reset_index()
    res = res.groupby(group_cols).mean()
    res = res.reset_index()
    res = res.drop(general_drop, axis=1)
    write_file(res, "_general.csv")  

def create_summary(data_table):
    summaries = [ops_col] + time_cols
    for summary in summaries:
        create_summary_table(data_table, summary)

    create_general_summary(data_table)

def analyze_data():
    data = get_notebook()
    data_table = create_raw_table(data)
    create_summary(data_table)
    
    
def process_inputs():
    global write_to_file
    global filename
    if len(sys.argv) > 1:
        filename = sys.argv[1]
        write_to_file = True    

if __name__ == '__main__':
    process_inputs()
    analyze_data()

