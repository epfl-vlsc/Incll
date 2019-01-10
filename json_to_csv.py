import json
import sys
import pandas as pd
import os

ndir = "output"
exp_col = "exp_no"
write_to_file = True
ops_col = "ops"
test_col = "test"
ignore_keys = ["table", "trial", "mb"]
time_cols = ["thread_time", "workload_time"]
info_cols = ["keys", "threads", "num_operations", "delay_count"]
group_cols = info_cols + ["test"]
special_ops_cols = ["rem_ops", "scan_ops", "get_ops", "put_ops"]
misc_cols = ["thread", exp_col]
indiv_group_cols = group_cols + [exp_col]
general_drop = info_cols + special_ops_cols + misc_cols

def write_file(df, fname):  
    if write_to_file:    
        outfname = fname.replace(".json", ".csv")
        print("Write to " + outfname)
        df.to_csv(outfname, index=False)
    
def get_notebook(name):
    with open(os.path.join(name), 'r') as f:
        json_obj = json.load(f)
        data = json_obj["data"]
        return data

def learn_headers(data):
    data_dict = dict()
    for i, run in enumerate(data):
        for thread_dict in data[run]:
            for tkey, tval in thread_dict.iteritems():
                if tkey in ignore_keys:
                    continue
                
                data_dict[tkey] = []
            
    data_dict[exp_col] = []
    return data_dict

def fix_keys(d, exp_no):
    for ignore_key in ignore_keys:
        d.pop(ignore_key, None)
    d[exp_col] = exp_no


def create_raw_table(data, name):
    data_dict = learn_headers(data)
    df = pd.DataFrame(data_dict)    

    for i, run in enumerate(data):
        for thread_dict in data[run]:
            fix_keys(thread_dict, i)
            df = df.append(thread_dict, ignore_index=True)
      
    write_file(df, name)   


def analyze_data(name):
    print("Analyzing " + name)
    data = get_notebook(name) 
    create_raw_table(data, name)
      

def get_files():
    list_of_files = []
    for (dirpath, dirnames, filenames) in os.walk(ndir):
        for filename in filenames:
            if filename.endswith('.json'): 
                list_of_files.append(os.path.join(dirpath, filename))
    return list_of_files

def analyze_all_files():
    list_of_files = get_files()

    for fname in list_of_files:
        analyze_data(fname)

    

if __name__ == '__main__':    
    analyze_all_files()



