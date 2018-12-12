import pandas as pd

def read_csv(fname):
	try:
		wperf = pd.read_csv(fname)
		return wperf
	except Exception as e:
		print("File not found")
		exit(1)


baselinedir = "./../MasstreeOriginal/"
perfdir = "perf_output/"
zfile = "zmeta.txt"
outdir = perfdir + "zmetaout.txt"


totopsstr = 'TotalOps'
totimestr = 'TotalTime'
basestr = '_baseline'
pmassstr = '_incll'
oheadstr = '_overhead'

cs_file = baselinedir + perfdir + zfile
cs_incll_file = perfdir + zfile

wcs = read_csv(cs_file)
wcs_incll = read_csv(cs_incll_file)

df = pd.merge(wcs, wcs_incll, on=['Workload'], suffixes=(basestr, pmassstr))
df[totopsstr + oheadstr] = (1 - df[totopsstr + pmassstr] / df[totopsstr + basestr])
df[totimestr + oheadstr] = (1 - df[totimestr + basestr] / df[totimestr + pmassstr])

#print(df.groupby(["Workload"]).reset_index())
df.to_csv(outdir)


"""


df = df.T
df.to_csv(outdir, header=None)

"""
