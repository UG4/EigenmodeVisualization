#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

def convert_to_ascii(src, dest, dim):
	# create a new 'XML Unstructured Grid Reader'
	ev_1vtu = XMLUnstructuredGridReader(FileName=[src])

	if dim == "1":
		ev_1vtu.PointArrayStatus = ['displacement']

	if dim == "3":
		ev_1vtu.PointArrayStatus = ['ux', 'uy', 'uz']

	# save data
	SaveData(dest, proxy=ev_1vtu, DataMode='Ascii')

import sys

for i in range(2, len(sys.argv)):
	fin = sys.argv[i]
	if "ascii" in fin:
		continue
	fout = fin.replace(".vtu", "_ascii.vtu")
	convert_to_ascii(fin, fout, sys.argv[1])


