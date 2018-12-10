import pandas as pd

def read_csv(fname):
	try:
		wperf = pd.read_csv(fname, skiprows=2, header=None)
		return wperf
	except Exception as e:
		print("File not found")
		exit(1)
pd.options.mode.chained_assignment = None
perfdir = "perf_output/"

workloads = ["ycsb_a_uni", "ycsb_b_uni", "ycsb_c_uni", "ycsb_e_uni", 
	"ycsb_a_zipf", "ycsb_b_zipf", "ycsb_c_zipf", "ycsb_e_zipf"]

for workload in workloads:
	wfile = perfdir + workload + ".csv"
	wfile_skip = perfdir + workload + "_skip" + ".csv"
	wfile_cs = perfdir + workload + "_cs" + ".csv"
		
	wperf = read_csv(wfile)
	wperf_skip = read_csv(wfile_skip)
	
	#assume last line is elapsed time for 10 runs
	wperf_cs_results = wperf[0] - wperf_skip[0]
	wperf_cs_results.iloc[-1] = wperf_cs_results.iloc[-1] / 10
	wperf_cs_labels = wperf[2]	
	wperf_cs_labels.iloc[-1] = "real-elapsed-time-avg(secs)"
							   
	
	wperf_cs = pd.DataFrame({workload:wperf_cs_results, "PMU":wperf_cs_labels})
	wperf_cs.to_csv(wfile_cs, index=False)
