#include "aio_files_reader.h"	
#include <cstdio>
#include <vector>
#include <chrono>					// now()

// ---------------------------
struct TRequest {
	char  name[256];
	int   size;
	char  contents;
	char* buf;
	int   handle;
};

// Yes, these are some globals
std::vector< TRequest > requests;
CAIOFilesReader aiofiles(256);

// ----------------------------------------------
bool readAllAsync( ) {

	auto start = std::chrono::steady_clock::now();

	// queueing takes most of the time in my tests
	for( auto& r : requests ) {
		r.handle = aiofiles.readAIO( r.name, r.buf, r.size, &r );
		if( r.handle < 0 ) {
			printf( "Failed to start reading file %s : %d\n", r.name, r.handle );
			return false;
		}
	}

	int total_bytes = 0;
	int ndone = 0;
	while( ndone != requests.size() ) {

		if( aiofiles.wait( )) {
			for( auto user_data : aiofiles.completed_items ) {
				auto item = (TRequest*) user_data;
				ndone++;
				total_bytes += item->size;
			}
			//printf( "Completed %ld/%ld handles: %ld/%ld bytes\n", completed_handles.size(), requests.size(), total_bytes, total_bytes_to_read );
		}
	}

	auto duration = std::chrono::duration_cast< std::chrono::microseconds > (std::chrono::steady_clock::now() - start);
	printf( "AIO read completed in %ld us\n", duration);
	return true;
}

// Read each file using old-school c style
bool readAllSync( ) {
	auto start = std::chrono::steady_clock::now();
	for( auto& r : requests ) {
		FILE *f = fopen( r.name, "rb" );
		if( !f )
			return false;
		auto bytes_read = fread( r.buf, 1, r.size, f );
		if( bytes_read != r.size )
			return false;
		fclose( f );
	}
	auto duration = std::chrono::duration_cast< std::chrono::microseconds > (std::chrono::steady_clock::now() - start);
	printf( "Sync read completed in %ld us\n", duration);
	return true;
}

// -------------------------------------------
// Confirm the buffers read are OK
void checkFiles() {
	for( auto&r : requests ) {
		// Check if all bytes have the expected value
		for( int i=0; i<r.size; ++i) {
			if( r.buf[i] != r.contents ) {
				printf( "Content of file %s failed. At index %d should contain %02x but contains %02x\n", r.name, i, r.contents, (unsigned char) r.buf[i]);
				return;
			}
		}
		// Clear the buffer
		memset( r.buf, 0, r.size );
	}
	printf( "All ok\n");
}

bool createFiles(int nfiles) {
	printf( "Creating %d dummy files\n", nfiles);
	for( auto& r : requests ) {
		FILE *f = fopen( r.name, "wb" );
		if( !f )
			return false;
		memset( r.buf, r.contents, r.size );
		auto bytes_written = fwrite( r.buf, 1, r.size, f );
		memset( r.buf, 0x00, r.size );
		if( bytes_written != r.size ) {
			printf( "Failed creating file %s\n", r.name );
			return false;
		}
		fclose( f );
	}
	return true;
}

// ------------------------------------------------
int main(int argc, char** argv) {

	int nfiles = 256;

	// Prepare the data
	size_t total_bytes_to_read = 0;
	for( int i=0; i<nfiles; ++i ) {
		TRequest r;
		sprintf( r.name, "data/data%03d.bin", i );
		r.size = 100 + i * 100;
		r.contents = i & 255;
		r.buf = new char[ r.size ];
		total_bytes_to_read += r.size;
		requests.push_back( r );
	}

	// If no arguments are given, create the files
	if( argc == 1 ) {
		if( !createFiles( nfiles ) )
			return -2;
	}

	readAllAsync();	
	checkFiles();
	readAllSync();
	checkFiles();

	return 0;
}
