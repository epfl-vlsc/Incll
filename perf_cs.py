import pandas as pd

def read_csv(fname):
	try:
		wperf = pd.read_csv(fname, skiprows=2, header=None)
		return wperf
	except Exception as e:
		print("File not found")
		exit(1)

perfdir = "perf_output/"

workloads = ["ycsb_a_uni", "ycsb_b_uni", "ycsb_c_uni", "ycsb_e_uni", 
	"ycsb_a_zipf", "ycsb_b_zipf", "ycsb_c_zipf", "ycsb_e_zipf"]

for workload in workloads:
	wfile = perfdir + workload + ".csv"
	wfile_skip = perfdir + workload + "_skip" + ".csv"
	wfile_cs = perfdir + workload + "_cs" + ".csv"
		
	wperf = read_csv(wfile)
	wperf_skip = read_csv(wfile_skip)
	wperf_cs = pd.DataFrame({workload:wperf[0] - wperf_skip[0], "PMU":wperf[2]})

	
	wperf_cs.to_csv(wfile_cs, index=False)
