# aio_api
Testing performance of reading files using the POSIX asynchronous I/O api (aio)

# Compile
	mkdir -p data
	cc main.cpp -std=c++11 -lstdc++ -lrt -O3

# Run: 
./a.out 
Creating 256 dummy files
AIO read completed in 4009 us
All ok
Sync read completed in 10802 us
All ok

# Conclusions
	Execution times are not really coherent between each run, even when aio api seems to be
	faster than sync files, sometimes this is not always true.
	Changing the file size does not affect the results.
	Probably the environment is not really appropiate running in a VM machine with a SSD drive in the host.