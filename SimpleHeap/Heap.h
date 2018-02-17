#ifndef _HEAP_H_
#define _HEAP_H_

#include "types.h"
#include "GCAssert.h"

//TODOS @
// 1) Check int / pointer size gurantees. Int is not guranteed to be able to hold a pointer on all systems.
//		See: https://stackoverflow.com/questions/153065/converting-a-pointer-into-an-integer
// 2) Careful where assumptions are made about big endian and little endian systems. (N.B. Gamercamp system is little-endian)
// 3) What should the system do in the case where an int cannot store a pointer value and vice-versa?
// 4) Switch C-Style casts to reinterpret-cast. Prevent additional code generation.

//////////////////////////////////////////////////////////////////////////
// default alignment for windows platform is sizeof(u32)
//////////////////////////////////////////////////////////////////////////
#define CHEAP_PLATFORM_MIN_ALIGN	(sizeof(u32))

class CHeap
{
public:
	//////////////////////////////////////////////////////////////////////////
	// enum of possible error return values from CHeap::GetLastError()
	// These are grouped into errors relating to specific functionality
	//////////////////////////////////////////////////////////////////////////
	enum EHeapError
	{		
		EHeapError_Ok = 0,						// no error
		EHeapError_NotInitialised,				// not yet initialised, if user tries to pass a buffer LTE (iMaxAllocs * sizeof(void*)), it will be rejected.
		EHeapError_Init_BadAlign,				// memory passed Initialise wasn't aligned to CHEAP_PLATFORM_MIN_ALIGN 
		EHeapError_Alloc_WarnZeroSize,			// allocation of 0 bytes requested (valid, but might indicate bug)
		EHeapError_Alloc_BadAlign,				// alignment specified is < CHEAP_PLATFORM_MIN_ALIGN or *not a power of 2* 
		EHeapError_Alloc_NoLargeEnoughBlocks,	// can't find a large enough free block to allocate requested size
		EHeapError_Alloc_MaxAllocsReached,		// maximum number of allocations reached for this CHeap, allocation ignored
		EHeapError_Dealloc_WarnNULL,			// warn NULL passed for dealloc
		EHeapError_Dealloc_AlreadyDeallocated,	// tried to deallocate a block that's already deallocated
		EHeapError_Dealloc_OverwriteUnderrun,	// memory overwrite was detected before the allocated block 
		EHeapError_Dealloc_OverwriteOverrun,	// memory overwrite was detected after the allocated block 
	};

	//////////////////////////////////////////////////////////////////////////
	// enum of the possible policies the CHeap will try and use to get a 
	// free block of memory from the allocated region. Default is FirstAvail.
	//////////////////////////////////////////////////////////////////////////
	enum EAllocPolicy
	{
		BestFit,		// Try to allocate to the smallest available fitting block. Worst time complexity. NOT IMPLEMENTED!
		WorstFit,		// Try to allocate to the largest available fitting block. Least fragmentation. NOT IMPLEMENTED!
		FirstAvail		// Get the first encountered fitting block. Best time complexity.
	};


	// default constructor and destructor
			CHeap			( void );
			~CHeap			( void );

	// explicit construction and destruction
	// N.B The heap deletes the assigned memory.
	void	Initialise		( u8* pRawMemory, u32 uMemorySizeInBytes, u32 iMaxAllocs = sm_kuDefaultMaxAllocs, EAllocPolicy eAllocPolicy = EAllocPolicy::FirstAvail );
	void	Shutdown		( void );

	// allocate deallocate
	void*	Allocate		( u32 uNumBytes );
	void*	AllocateAligned	( u32 uNumBytes, u32 uAlignment );
	void 	Deallocate		( void* pMemory );

	// get info about the current Heap state
	u32		GetNumAllocs	( void );

	// N.B. this should return ( size of heap - memory allocated including overheads )
	// Clearly this is slightly > maximum allocatable memory
	u32		GetFreeMemory	( void );	

	// if the last operation caused an error this should return a value != EHeapError_Ok
	EHeapError GetLastError	( void );

	// returns a POSITIVE value between 0 and (uAlignment-1) to add to the address of 
	// pPointerToAlign in order to align it to uAlignment
	static u32 CalculateAlignmentDelta( void* pPointerToAlign, u32 uAlignment );

private:
	//Static variables
	static const u32	sm_kuDefaultMaxAllocs = 32;				//The default maximum number of allocations a memory block will support, override on Initialise() if needed.
	static const u32	sm_kuConsistencyValue = 0xFACCFACC;		//The 4-byte integer that will be checked for in the header and footer blocks on consistency checks.
	static const u8		sm_kuConsistencyBlockSize = 8;			//The size of the consistency header/footer blocks in bytes. Should be a multiple of 4.
	static const u8		sm_kuNumberOfHeaderSequences = ( sm_kuConsistencyBlockSize / sizeof( sm_kuConsistencyValue ) );

	//Memory block pointer
	u8* m_pRawMemory;

	//Memory block size
	u32 m_uMemorySizeInBytes;

	//Memory allocation tracking array - created on initialisation. The u32 values are pointer addresses.
	//N.B. pointer sizes change with the machine type. 4 bytes on 32-bit system.
	u32* m_pAllocArray;

	//Maximum number of allocations - default is sm_kuDefaultMaxAllocs
	u32 m_uMaxAllocs;

	//Current number of memory blocks allocated - decremented on deallocation.
	u32 m_uAllocCount;

	//The most recent error we encountered.
	EHeapError m_eLastError;

	//Allocation policy for free memory. Default is FirstAvail if not overriden.
	EAllocPolicy m_eAllocPolicy;

	//Constants for actual memory size / block size.
	u32 m_uAllocatableMemorySizeInBytes;
	u32 m_uBlockSizeInBytes;
	u8* m_pBaseAllocatableAddress;

	//Locate a suitable 'free' memory block for a given allocation size. Applies m_eAllocPolicy.
	//This method returns the ARRAY location pointer, not the memory block head.
	u32* GetFreeBlockLocation( u32 uNumBytes );

	u32 GetArrayIndexFromArrayPtr( u32* pArrayPtr );

	u32* GetBlockHeadFromArray( u32* pArrayPtr );

	u32 GetNumberOfOccupiedBlocks();

	//Allocation method for each allocation policy.
	u32* FirstAvailBlock	( u32 uNumBytes );
	u32* BestFitBlock		( u32 uNumBytes );
	u32* WorstFitBlock		( u32 uNumBytes );

	//Debug checks
	bool WasMemoryUnderrun	( u8* pHeadLocation );
	bool WasMemoryOverrun	( u8* pHeadLocation, u32 uNumberOfBlocks );

};
#endif // #ifndef _HEAP_H_