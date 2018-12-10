import pandas as pd

def read_csv(fname):
	try:
		wperf = pd.read_csv(fname)
		return wperf
	except Exception as e:
		print("File not found")
		exit(1)

workloads = ["ycsb_a_uni", "ycsb_b_uni", "ycsb_c_uni", "ycsb_e_uni", 
	"ycsb_a_zipf", "ycsb_b_zipf", "ycsb_c_zipf", "ycsb_e_zipf"]

baselinedir = "./../MasstreeOriginal/"
perfdir = "perf_output/"
outdir = perfdir + "zcs.csv"

basestr = '_baseline'
pmassstr = '_incll'

df = None
first = True
for workload in workloads:
	cs_file = baselinedir + perfdir + workload + "_cs" + ".csv"
	cs_incll_file = perfdir + workload + "_cs" + ".csv"
		
	wcs = read_csv(cs_file)
	wcs_incll = read_csv(cs_incll_file)
	
	result = pd.merge(wcs, wcs_incll, on=['PMU'], suffixes=(basestr, pmassstr))
	result[workload+"_overhead"] = 1 - result[workload+basestr]/result[workload+pmassstr]

	if first:
		df = result
		first = False
	else:
		df = pd.merge(df, result, on=['PMU'])


df = df.T
df.to_csv(outdir, header=None)


