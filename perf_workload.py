import pandas as pd

def read_csv(fname):
	try:
		wperf = pd.read_csv(fname)
		return wperf
	except Exception as e:
		print("File not found")
		exit(1)

baselinedir = "./../MasstreeOriginal/"
perfdir = "output/"
outfile_baseline = baselinedir + perfdir + "workload_perf.txt"
outfile_incll = perfdir + "workload_perf.txt"

basestr = '_baseline'
pmassstr = '_incll'

wcs_baseline = read_csv(outfile_baseline)
wcs_incll = read_csv(outfile_incll)

result = pd.merge(wcs_baseline, wcs_incll, on=['Workload'], suffixes=(basestr, pmassstr))

counter_names = [
    "instructions", "cycles", 
    "l1dc_loadreferences", "l1dc_loadmisses", 
    "llc_loadreferences","llc_loadmisses"
]

for counter_name in counter_names:
    cname = "Total_" + counter_name
    result[cname + "_overhead"] = 1 - result[cname+pmassstr]/result[cname+basestr]

df = result
df.to_csv(outdir)


