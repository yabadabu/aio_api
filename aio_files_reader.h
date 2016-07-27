#ifndef INC_AIO_FILES_READER_H_
#define INC_AIO_FILES_READER_H_

#include <cassert>
#include <vector>
#include <cstring>				// memset
#include <unistd.h>				// close
#include <sys/fcntl.h>		// open
#include <errno.h>				// EINPROGRESS
#include <aio.h>

// -------------------------------------------------------
class CAIOFilesReader {
	std::vector< aiocb  > items;
	std::vector< aiocb* > active_items;
	std::vector< aiocb* > free_items;
	std::vector< void* >  user_datas;

	// given an addr, can provide a unique index
	int getHandleFromAddr( aiocb* addr ) const {
		int h = addr - &items[0];
		assert( h >= 0 && h < items.size() );
		return h;
	}

public:

	std::vector< void* >  completed_items;

	CAIOFilesReader(int max_files) {

		// Clear everything
		items.resize( max_files );
		memset( &items[0], 0x00, items.size() * sizeof( aiocb ));

		// Prepare two lists of pointers... All are free now
		free_items.reserve( max_files );
		active_items.reserve( max_files );
		for( int i=0; i<max_files; ++i )
			free_items.push_back( &items[i] );

		completed_items.reserve( max_files );
		user_datas.resize( max_files );
	}

	size_t wait( ) {
		//printf( "Checking in %ld files\n", active_items.size());
		if( active_items.empty() )
			return 0;

		completed_items.resize( 0 );

		// Will sleep until some file gets ready
		auto rc = aio_suspend( &active_items[0], active_items.size(), nullptr );
		assert( rc == 0 );

		// Scan for changes
		int idx = 0;
		while( idx < active_items.size() ) {
			auto item = active_items[ idx ];			
			auto err = aio_error( item );
			
			if( err != 0 ) {
				if( err == EINPROGRESS ) {
					// Still ongoing..?
				} else {
					// Some other error
				}
				++idx;

			} else {

				int handle = getHandleFromAddr( item );

				// The user wants his own user_data
				completed_items.push_back( user_datas[ handle ] );

				// undoing the open
				close( item->aio_fildes );

				// Move the last one in this position, unless we are dealing with the last one
				if( idx != active_items.size()-1 )
					active_items[idx] = active_items.back();
				
				// pop the last one instead of idx++
				active_items.resize( active_items.size()-1 );

				// This slot is now free
				free_items.push_back( item );
			}
		}
		return completed_items.size();
	}

	// Starts reading op on file ifilename into out_buffer
	int readAIO( const char* ifilename, void* out_buffer, size_t out_buffer_size, void* user_data ) {

		if( free_items.empty())
			return -2;

		// Confirm the file exists
		auto fd = open( ifilename, 0 );
		if( fd == 0 )
			return -1;

		// pop the last item from the free items lists
		aiocb* item = free_items.back();
		free_items.resize( free_items.size() - 1);

		item->aio_fildes = fd;
		item->aio_offset = 0;
		item->aio_buf    = out_buffer;
		item->aio_nbytes = out_buffer_size;
		item->aio_reqprio = 0;
		item->aio_sigevent.sigev_notify = SIGEV_NONE;

		if( aio_read( item ) != 0 )
			return -2;

		// push in the active list
		active_items.push_back( item );

		// Get an index associated to the item slot
		int handle = getHandleFromAddr( item );

		// Bind the user_data pointer to our item
		user_datas[ handle ] = user_data;

		return handle; 
	}

};

#endif
