import pandas as pd
import StringIO

def read_csv(fname):
	try:
		wperf = pd.read_csv(fname, skiprows=2, header=None)
		return wperf
	except Exception as e:
		print("File not found")
		exit(1)

workloads = ["ycsb_a_uni", "ycsb_b_uni", "ycsb_c_uni", "ycsb_e_uni", 
	"ycsb_a_zipf", "ycsb_b_zipf", "ycsb_c_zipf", "ycsb_e_zipf"]

baselinedir = "./../MasstreeOriginal/"
perfdir = "perf_output/"
outdir = perfdir + "z_raw.csv"

basestr = '_baseline'
pmassstr = '_incll'
fullstr = '_full'
skipstr = '_skip' 



with open(outdir, "w") as fw:
	for workload in workloads:
		files = [
			baselinedir + perfdir + workload + ".csv",
			baselinedir + perfdir + workload + "_skip" + ".csv",
			perfdir + workload + ".csv",
			perfdir + workload + "_skip" + ".csv"
		]
		tags = [
			workload + basestr + fullstr,
			workload + basestr + skipstr,
			workload + pmassstr + fullstr,
			workload + pmassstr + skipstr,
		]	
		
		for fr, tag in zip(files, tags):
			wperf = read_csv(fr) 		
			s = StringIO.StringIO()
			wperf.to_csv(s, header=None, index=None)
			
			fw.write("{}\n".format(tag))
			fw.write(s.getvalue())
		

		

	
