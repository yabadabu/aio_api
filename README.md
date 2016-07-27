# aio_api
Testing performance of reading files using the POSIX asynchronous I/O api (aio)

# Compile
	mkdir -p data
	cc main.cpp -std=c++11 -lstdc++ -lrt -O3

# Example output of 256 between 100 bytes and 25600 bytes
	./a.out 
	Creating 256 dummy files
	AIO read completed in 4009 us
	All ok
	Sync read completed in 10802 us
	All ok

# Conclusions
Execution times are not really coherent between each run, even when aio api seems to be faster than sync reads, sometimes this is not true.

Changing the file size does not affect the results.

Probably the test environment is not really appropiate (Linux VM machine running on a Win10 host machine + SSD drive)

# Todo
More error checking and reporting api
Testing it on other scenarios.
