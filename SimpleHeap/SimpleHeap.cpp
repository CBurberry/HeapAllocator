// SimpleHeap.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <iostream>
#include "Heap.h"



int _tmain(int argc, char argv[])
{
	//////////////////////////////////////////////////////////////////////////
	// feel free to add any of your own test code below this comment
	//////////////////////////////////////////////////////////////////////////

		// please comment or #define out any test code you 
		// add here before checking in your project

	//////////////////////////////////////////////////////////////////////////
	// feel free to add any of your own test code above this comment
	//////////////////////////////////////////////////////////////////////////

	const u32 k_uHeapSize	= 2048;
	u8* pHeapMemory			= new u8[ k_uHeapSize ];

	// create the heap
	CHeap cTestHeap;
	
/*
	std::cout << "Hello world!" << std::endl;

	cTestHeap.Initialise( pHeapMemory, k_uHeapSize );
	
	std::cout << "Error code: " << cTestHeap.GetLastError() << std::endl;

	/*cTestHeap.Allocate( 1 );

	std::cout << "Error code: " << cTestHeap.GetLastError() << std::endl;
*/
//	__debugbreak();

	/*while ( cTestHeap.GetLastError() == 0)
	{
		cTestHeap.Allocate( 128 );
		std::cout << "Error code: " << cTestHeap.GetLastError() << std::endl;
	}*/

//	__debugbreak();
	

	// test 1: EHeapError_NotInitialised
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// CHeap should not be usable until Initialise() has been called.
	// 
	// N.B. more or less all the tests rely on getting the correct error code from cTestHeap.GetLastError() so you must make sure that 
	// you set them correctly as well as actually make your simple heap allocator do the work it should
	{
		std::cout << "test 1: EHeapError_NotInitialised...";

		void* pNotInitYet = cTestHeap.Allocate( 69 );

		GCASSERT( CHeap::EHeapError_NotInitialised == cTestHeap.GetLastError(), "failed test 1: EHeapError_NotInitialised" );

		std::cout << "ok\n";
	}
	
	// test 2: EHeapError_Init_BadAlign
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// Initialse should fail if the memory passed to it is not aligned to a power of 2 >= CHEAP_PLATFORM_MIN_ALIGN  
	{
		std::cout << "test 2: EHeapError_Init_BadAlign...";

		cTestHeap.Initialise( &pHeapMemory[1], (k_uHeapSize - 1) );

		GCASSERT( CHeap::EHeapError_Init_BadAlign == cTestHeap.GetLastError(), "failed test 2: EHeapError_Init_BadAlign" );

		std::cout << "ok\n";
	}

	// initialise with memory with appropriate memory alignment 
	{
		std::cout << "initialise properly...";

		cTestHeap.Initialise( pHeapMemory, k_uHeapSize );

		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "error code not EHeapError_Ok after Initialise()" );

		std::cout << "ok\n";
	}

	// test 3: basic test
	// ^^^^^^^^^^^^^^^^^^
	// This tests that a pointer returned from Allocate() can be passed to Deallocate() without causing error
	{
		std::cout << "test 3: basic test...";
		void* pBasic = cTestHeap.Allocate( 32 );
		GCASSERT( nullptr != pBasic, "failed test 3: basic test - Allocate returned nullptr" );
		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "failed test 3: basic test - error code not EHeapError_Ok after alloc" );

		cTestHeap.Deallocate( pBasic );
		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "failed test 3: basic test - error code not EHeapError_Ok after dealloc" );
		std::cout << "ok\n";
	}

	// test 4: stress test
	// ^^^^^^^^^^^^^^^^^^^
	// This tests that: 
	// * allocation and deallocation work when memory is deallocated in an aribtrary order
	// * that the overheads on allocation are not excessive
	// * and that adjacent free memory blocks are joined together into larger blocks
	{
		std::cout << "test 4a: stress test...";
		// N.B. 20 * 32 = 640, leaves a spare 1408 for overheads out of the 2048 initially allocated
        // ... enough for up to 64 bytes overhead per alloc	(you shouldn't need anywhere near this much!)
		const u32	k_uStress_Allocations		= 20; 
		const u32	k_uStress_AllocationSize	= 32;
		void*		apStressAllocations[ k_uStress_Allocations ];

		GCASSERT( 0 == cTestHeap.GetNumAllocs(), "failed test 4: stress test - allocations present before we started!" );

		// allocate
		for( u32 uLoop = 0; uLoop < k_uStress_Allocations; ++uLoop )
		{
			std::cout << "\n    allocating [" << uLoop << "]...";

			apStressAllocations[ uLoop ] = cTestHeap.Allocate( k_uStress_AllocationSize );

			GCASSERT( nullptr != apStressAllocations[ uLoop ], "failed test 4: stress test - Allocate() returned nullptr [uLoop %d]", uLoop );
			GCASSERT( cTestHeap.GetNumAllocs() == ( uLoop + 1 ), "failed test 4: stress test - GetNumAllocs() doesn't match uLoop [uLoop %d]", uLoop );
			GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "failed test 4: stress test - error code not EHeapError_Ok after alloc" );

			std::cout << "ok";
		}

		// check free space < (size of memory - sizeof allocs we made). This is caused by per alloc overheads
		GCASSERT(	(cTestHeap.GetFreeMemory() < ( k_uHeapSize - ( k_uStress_Allocations * k_uStress_AllocationSize ))),
					"failed test 4: stress test... - more free space than there should be! [should be < %d]", (k_uHeapSize - ( k_uStress_Allocations * k_uStress_AllocationSize )) );

		// deallocate - 1st 8 semi-random dealloc order
		u32 auDeallocIndex[ 8 ] = { 4, 2, 5, 3, 7, 1, 6, 0 }; 
		for( u32 uLoop = 0; uLoop < ( sizeof(auDeallocIndex) / sizeof(u32) ); ++uLoop )
		{
			std::cout << "\n    deallocating [" << auDeallocIndex[ uLoop ] << "]...";

			cTestHeap.Deallocate( apStressAllocations[ auDeallocIndex[ uLoop ] ] );

			GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "failed test 4: stress test - error code not EHeapError_Ok after dealloc" );

			std::cout << "ok";
		}

        // deallocate remaining in same order allocated 
		for( u32 uLoop = ( sizeof(auDeallocIndex) / sizeof(u32) ); uLoop < k_uStress_Allocations; ++uLoop )
		{
			std::cout << "\n    deallocating [" << uLoop << "]...";

			cTestHeap.Deallocate( apStressAllocations[ uLoop ] );

			GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "failed test 4: stress test - error code not EHeapError_Ok after dealloc" );

			std::cout << "ok";
		}

		// check no allocs and free space == max memory size
		//
        // N.B. for the sake of this test the system should report the maximum size of all non-allocated memory in the free list 
        // even though this is slightly larger than the maximum memory that can be allocated
        // 
        // N.N.B. in "real life" we would probably also have the memory manager also return the minimum allocation overhead 
        // so client code can subtract that from the reported free memory to make an informed decision about whether an alloc 
        // is likely to work...
		GCASSERT( 0 == cTestHeap.GetNumAllocs(), "failed test 4: stress test - allocations present after all memory deallocated!" );
		GCASSERT(	( cTestHeap.GetFreeMemory() == k_uHeapSize ),
					"failed test 4: stress test... - free space should be max size [should be == %d]", k_uHeapSize );


		// test coalescing of free blocks (i.e. that adjacent free blocks are being merged together)
		std::cout << "\n    allocating a big chunk after all the small deallocs...";	

		void* pNeedsBigChunk = cTestHeap.Allocate( 1900 );

		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "failed test 4: stress test - error code not EHeapError_Ok after theoretically small enough alloc" );

		cTestHeap.Deallocate( pNeedsBigChunk );

		std::cout << "ok\n";
	}

	// test 5: EHeapError_Alloc_WarnZeroSize
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// This checks that a request for 0 bytes returns a pointer that can be deallocated, and that a warning is given
	std::cout << "test 5: EHeapError_Alloc_WarnZeroSize...";
	// N.B. This is valid code. Should return a valid pointer to memory that can be deallocated.
	{
		void* pZeroAlloc = cTestHeap.Allocate( 0 );

		GCASSERT( nullptr != pZeroAlloc, "failed test 5: EHeapError_Alloc_WarnZeroSize - Allocate returned nullptr" );
		GCASSERT( CHeap::EHeapError_Alloc_WarnZeroSize == cTestHeap.GetLastError(), "failed test 5: EHeapError_Alloc_WarnZeroSize - allocate" );

		cTestHeap.Deallocate( pZeroAlloc );

		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "failed test 5: EHeapError_Alloc_WarnZeroSize - deallocate" );
		std::cout << "ok\n";
	}

	// test 6: EHeapError_Alloc_BadAlign
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// tests that allocator will only accept power of 2 alignment values
	{
		std::cout << "test 6: EHeapError_Alloc_BadAlign...";

		void* pBadAlignRequest = cTestHeap.AllocateAligned( 42, 13 );

		GCASSERT( nullptr == pBadAlignRequest, "failed test 6: EHeapError_Alloc_BadAlign - returned non-nullptr" );
		GCASSERT( CHeap::EHeapError_Alloc_BadAlign == cTestHeap.GetLastError(), "failed test 6: EHeapError_Alloc_BadAlign" );
		std::cout << "ok\n";
	}
/*
	// test 6a: aligned alloc
	// ^^^^^^^^^^^^^^^^^^^^^^
	// Tests that a non CHEAP_PLATFORM_MIN_ALIGN alignment does not cause an error
	{
		std::cout << "test 6a: aligned alloc...";
		const u32 k_uAlignment = 16;

		void* pAligned = cTestHeap.AllocateAligned( 32, k_uAlignment );

		GCASSERT( nullptr != pAligned, "failed test 6a: aligned alloc - AllocateAligned returned nullptr" );
		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "failed test 6a: aligned alloc - error code not EHeapError_Ok after alloc" );
		GCASSERT( (0 == CHeap::CalculateAlignmentDelta( pAligned, k_uAlignment )), "failed test 6a: aligned alloc - alloc not correctly aligned!" );

		cTestHeap.Deallocate( pAligned );

		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "failed test 6a: aligned alloc - error code not EHeapError_Ok after dealloc" );
		std::cout << "ok\n";
	}

	// test 6b: check internal alignment works with unaligned allocation sizes
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// checks that consecutive allocs of unaligned size don't break the allocator
	{
		std::cout << "test 6b: internal alignment consistency...";
		const u32 k_uAlignment = 64;

		void* pAligned = cTestHeap.AllocateAligned( 9, k_uAlignment );
		
		GCASSERT( nullptr != pAligned, "test 6b: internal alignment consistency - AllocateAligned returned nullptr (pAligned)" );
		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "test 6b: internal alignment consistency - error code not EHeapError_Ok after alloc pAligned" );
		GCASSERT( (0 == CHeap::CalculateAlignmentDelta( pAligned, k_uAlignment )), "test 6b: internal alignment consistency - alloc of pAligned not correctly aligned to %d byte boundary", k_uAlignment );

		void* pAlignedTwo = cTestHeap.AllocateAligned( 13, k_uAlignment );

		GCASSERT( nullptr != pAlignedTwo, "test 6b: internal alignment consistency - AllocateAligned returned nullptr (pAlignedTwo)" );
		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "test 6b: internal alignment consistency - error code not EHeapError_Ok after alloc pAlignedTwo" );
		GCASSERT( (0 == CHeap::CalculateAlignmentDelta( pAlignedTwo, k_uAlignment )), "test 6b: internal alignment consistency - alloc of pAlignedTwo not correctly aligned to %d byte boundary", k_uAlignment );

		cTestHeap.Deallocate( pAligned );
		
		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "test 6b: internal alignment consistency - error code not EHeapError_Ok after dealloc pAligned" );
		
		cTestHeap.Deallocate( pAlignedTwo );
		
		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "test 6b: internal alignment consistency - error code not EHeapError_Ok after dealloc pAlignedTwo" );
		
		std::cout << "ok\n";
	}
*/
	// test 7: EHeapError_Alloc_OutOfMemory / EHeapError_Alloc_NoLargeEnoughBlocks
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// checks that the allocator returns the correct error code when unable to allocate
	{
		std::cout << "test 7: EHeapError_Alloc_OutOfMemory / EHeapError_Alloc_NoLargeEnoughBlocks...";
		void* pTooBigAlloc = cTestHeap.Allocate( 2048 );
		GCASSERT( nullptr == pTooBigAlloc, "test 7: EHeapError_Alloc_NoLargeEnoughBlocks - Allocate returned non nullptr" );
		GCASSERT( CHeap::EHeapError_Alloc_NoLargeEnoughBlocks == cTestHeap.GetLastError(), "test 7: EHeapError_Alloc_NoLargeEnoughBlocks" );
		std::cout << "ok\n";
	}
			
	// test 8: EHeapError_Dealloc_WarnNULL
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// checks that the allocator warns when a nullptr is passed for deallocation
	{
		std::cout << "test 8: EHeapError_Dealloc_WarnNULL...";
		// N.B. This is valid code. Must handle a nullptr pointer.
		cTestHeap.Deallocate( nullptr );
		GCASSERT( CHeap::EHeapError_Dealloc_WarnNULL == cTestHeap.GetLastError(), "test 8: EHeapError_Dealloc_WarnNULL - wrong error on dealloc( nullptr )" );
		std::cout << "ok\n";
	}

	// test 9: EHeapError_Dealloc_AlreadyDeallocated
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// This checks whether the correct error is returned when a pointer that has already 
	// been deallocated is passed to Deallocate()
	// 
	// N.B. this is a debug feature & don't need to worry about performance
	// would never have it in release code
	{
		std::cout << "test 9: EHeapError_Dealloc_AlreadyDeallocated...";
		void* pAllocToDealloc = cTestHeap.Allocate( 123 );
		cTestHeap.Deallocate( pAllocToDealloc );
		GCASSERT( CHeap::EHeapError_Ok == cTestHeap.GetLastError(), "test 9: EHeapError_Dealloc_AlreadyDeallocated - regular dealloc failed" );

		cTestHeap.Deallocate( pAllocToDealloc );
		GCASSERT( CHeap::EHeapError_Dealloc_AlreadyDeallocated == cTestHeap.GetLastError(), "test 9: EHeapError_Dealloc_AlreadyDeallocated" );
		std::cout << "ok\n";
	}

	// test 10: EHeapError_Dealloc_OverwriteUnderrun
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// This checks the correct error code is returned when client code has overwritten 
	// memory immediately before the returned pointer (underwrite)
	// 
	// N.B. this is a debug feature & don't need to worry about performance - would never have it in release code
	{
		std::cout << "test 10: EHeapError_Dealloc_OverwriteUnderrun...";
		u32* puUnderWrite = reinterpret_cast< u32* >( cTestHeap.Allocate( sizeof( u32 ) ) );
		puUnderWrite[ -1 ] = 666;
		cTestHeap.Deallocate( puUnderWrite );
		GCASSERT( CHeap::EHeapError_Dealloc_OverwriteUnderrun == cTestHeap.GetLastError(), "test 10: EHeapError_Dealloc_OverwriteUnderrun - failed to detect underrun" );
		std::cout << "ok\n";
	}

	// test 11: EHeapError_Dealloc_OverwriteOverrun
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// This checks the correct error code is returned when client code has overwritten 
	// memory immediately after the allocated memory (overwrite)
	// 
	// N.B. this is a debug feature & don't need to worry about performance - would never have it in release code
	{
		std::cout << "test 11: EHeapError_Dealloc_OverwriteOverrun...";
		u32* puOverWrite = reinterpret_cast< u32* >( cTestHeap.Allocate( sizeof( u32 ) ) );
		puOverWrite[ 1 ] = 666;
		cTestHeap.Deallocate( puOverWrite );
		GCASSERT( CHeap::EHeapError_Dealloc_OverwriteOverrun == cTestHeap.GetLastError(), "test 11: EHeapError_Dealloc_OverwriteOverru - failed to detect overrun" );
		std::cout << "ok\n";
	}

	// clean up the heap memory	and exit
	cTestHeap.Shutdown();
	
	system("pause");
	return 0;
}

