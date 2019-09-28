doit:
	python ./tools/paraview_binary_ascii_converter.py 1 ./solutions/*
	python ./tools/paraview_binary_ascii_converter.py 3 ./debug/*
	./tools/vtu_ugx_converter -c ./solutions/*ascii.vtu
	python ./tools/converter.py
