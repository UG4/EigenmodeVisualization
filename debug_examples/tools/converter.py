import os
import sys

for root, dir, files in os.walk("./debug/"):
	for f in files:
		if "ascii.vtu" in f:
			os.system("./tools/vtu_ugx_converter -c ./debug/" + f)
