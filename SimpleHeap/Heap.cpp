#include "stdafx.h"
#include <iostream>
#include <algorithm>
#include "GCASSERT.h"
#include "Heap.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Credit - http://www.graphics.stanford.edu/~seander/bithacks.html
#define IS_POWER_OF_TWO(n) ( n && !(n & (n - 1)) )

//////////////////////////////////////////////////////////////////////////
// responsible for default initialising any state variables used
// Memory managed by CHeap is passed in via Initialise(..)
//////////////////////////////////////////////////////////////////////////
CHeap::CHeap( void ) 
	: m_pRawMemory						(nullptr)
	, m_pAllocArray						(nullptr)
	, m_pBaseAllocatableAddress			(nullptr)
	, m_uMaxAllocs						(0)
	, m_uAllocCount						(0)
	, m_uMemorySizeInBytes				(0)
	, m_uAllocatableMemorySizeInBytes	(0)
	, m_uBlockSizeInBytes				(0)
	, m_eLastError						(EHeapError_NotInitialised)
	, m_eAllocPolicy					(EAllocPolicy::FirstAvail)
{	
}


//////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////
CHeap::~CHeap( void )
{
}



//////////////////////////////////////////////////////////////////////////
// This function initialises the memory managed by CHeap.
// CHeap is not usable before this function has been called and 
// CHeap::GetLastError() returns EHeapError_Ok
// pRawMemory must be aligned to a power of 2 >= CHEAP_PLATFORM_MIN_ALIGN
// pRawMemory is a 1-byte array of size uMemorySizeInBytes.
// TODO: Initialise should only take power of two iMaxAllocs as argument.
//////////////////////////////////////////////////////////////////////////
void CHeap::Initialise( u8* pRawMemory, u32 uMemorySizeInBytes, u32 iMaxAllocs, EAllocPolicy eAllocPolicy )
{
	//If the memory size a power of two GTE CHEAP_PLATFORM_MIN_ALIGN and is GT the reserved memory size.
	if ( IS_POWER_OF_TWO( uMemorySizeInBytes ) 
		&& uMemorySizeInBytes >= CHEAP_PLATFORM_MIN_ALIGN 
		&& uMemorySizeInBytes > ( iMaxAllocs * sizeof(u32*) ) 
		&& CalculateAlignmentDelta(pRawMemory, CHEAP_PLATFORM_MIN_ALIGN) == 0 )
	{
		//Set members
		m_uMaxAllocs						= iMaxAllocs;
		m_pRawMemory						= pRawMemory;
		m_uMemorySizeInBytes				= uMemorySizeInBytes;
		m_uAllocatableMemorySizeInBytes		= m_uMemorySizeInBytes - ( m_uMaxAllocs * sizeof( u32 ) );
		m_uBlockSizeInBytes					= m_uAllocatableMemorySizeInBytes / m_uMaxAllocs;
		m_pBaseAllocatableAddress			= (u8*) ( (u32) m_pRawMemory ) + ( m_uMemorySizeInBytes - m_uAllocatableMemorySizeInBytes );

		//Initialise pointer array with placement new.
		m_pAllocArray = new ( pRawMemory ) u32 [iMaxAllocs];

		m_eLastError = EHeapError_Ok;

#ifdef _DEBUG
		//If we're in debug we want to clear all our assigned memory for footer tracking.
		std::fill(pRawMemory, pRawMemory + uMemorySizeInBytes, NULL);
#else
		//Set only array content memory to NULL
		std::fill( m_pAllocArray, m_pAllocArray + iMaxAllocs, NULL );
#endif
	}
	else 
	{
		//Set heap error
		m_eLastError = EHeapError_Init_BadAlign;
	}
}

//////////////////////////////////////////////////////////////////////////
// Explicit destructor. 
// The heap deletes the memory that was assigned to it.
//////////////////////////////////////////////////////////////////////////
void CHeap::Shutdown( void )
{
	delete[] m_pRawMemory;
	m_pRawMemory	= nullptr;
	m_pAllocArray	= nullptr;
}


//////////////////////////////////////////////////////////////////////////
// proxies the call to AllocateAligned() with default platform alignment
//////////////////////////////////////////////////////////////////////////
void* CHeap::Allocate( u32 uNumBytes )
{		
	void* puRtnAddress = nullptr;

	if ( GetLastError() != EHeapError_NotInitialised )
	{
		puRtnAddress = AllocateAligned( uNumBytes, CHEAP_PLATFORM_MIN_ALIGN );
	}
	return puRtnAddress;
}


//////////////////////////////////////////////////////////////////////////
// Allocates the specified size of memory, with the specified alignment
// and returns a pointer to it.
// 
// N.B. as extra data is required to track allocated & free memory blocks 
// the actual size of the chunk of memory you use out of your free memory 
// will be > uNumBytes
//////////////////////////////////////////////////////////////////////////
void* CHeap::AllocateAligned( u32 uNumBytes, u32 uAlignment )
{
	u32* puRtnAddress = nullptr;
	EHeapError eErrorValue = EHeapError_Ok;

	//N.B. 0 is also a power of two. This should return a valid pointer.
	//N.B. 1 is a power of two 2^0, this edge case should be ignored as not aligned.

	bool bIsAllocatableAlignment = IS_POWER_OF_TWO( uAlignment ) && uAlignment != 1;
	if ( bIsAllocatableAlignment )
	{
		if ( uNumBytes == 0 )
		{
			//Set heap error
			eErrorValue = EHeapError_Alloc_WarnZeroSize;
		}

		//Pad the actual size with the header and footer block size.
		u32 uFullSize = uNumBytes + ( sm_kuConsistencyBlockSize * sm_kuNumberOfHeaderSequences );

		//Get a free block that fits full size.
		u32* puFreeBlock = GetFreeBlockLocation( uFullSize );

		//Only allocate if the requested memory is servicable.
		if ( uFullSize <= m_uAllocatableMemorySizeInBytes && puFreeBlock != nullptr )
		{
			//If a free block is available that fits size & m_uAllocCount < max
			if ( m_uAllocCount < m_uMaxAllocs )
			{
				//Get the corresponding block head location from the array value returned.

				//Set header and footer block contents
				u32* pWriting4Byte = puFreeBlock;
				for ( u8 uLoop = 0; uLoop < sm_kuNumberOfHeaderSequences; ++uLoop ) 
				{
					*pWriting4Byte = sm_kuConsistencyValue;
					pWriting4Byte++;
				}
				pWriting4Byte = ( u32* ) ( ( (u32)puFreeBlock) + uFullSize );
				for ( u8 uLoop = 0; uLoop < sm_kuNumberOfHeaderSequences; ++uLoop )
				{
					pWriting4Byte--;
					*pWriting4Byte = sm_kuConsistencyValue;
				}

				//Increment count.
				m_uAllocCount++;

				//Set pRtnAddress to the free block location + uNumberOfHeaderSequences
				//pointer type addition multiplies the addition u8 by the size of pointer.
				puRtnAddress = puFreeBlock + sm_kuNumberOfHeaderSequences;
			}
			else 
			{
				//Set heap error
				eErrorValue = EHeapError_Alloc_MaxAllocsReached;
			}
		}
		else 
		{
			//Set heap error
			eErrorValue = EHeapError_Alloc_NoLargeEnoughBlocks;
		}
	}
	else
	{
		//Set heap error
		eErrorValue = EHeapError_Alloc_BadAlign;
	}

	m_eLastError = eErrorValue;
	return puRtnAddress;
}

//////////////////////////////////////////////////////////////////////////
// deallocates the memory pointed to by pMemory and returns it to the 
// free memory stored in the heap.
//////////////////////////////////////////////////////////////////////////
void CHeap::Deallocate( void* pMemory )
{
	//nullptr check.
	if ( pMemory == nullptr ) 
	{
		//Set heap error and exit.
		m_eLastError = EHeapError_Dealloc_WarnNULL;
		return;
	}

	EHeapError eErrorValue = EHeapError_Ok;

	//Get the real head location (pMemory - header), TODO - NOT u32 type safe!
	u32 uHeadLocation = ( (u32) pMemory ) - ( sm_kuNumberOfHeaderSequences * sizeof(u32) );
	u8* puHeadLocation = reinterpret_cast< u8* >( uHeadLocation );

	u32 uConsectutiveBlocks = 0;
	//Check the array for blocks that match head location value
	for ( unsigned int uLoop = 0; uLoop < m_uMaxAllocs; ++uLoop ) 
	{
		if ( m_pAllocArray[uLoop] == uHeadLocation )
		{
			m_pAllocArray[uLoop] = NULL;
			uConsectutiveBlocks++;
		}
		else if ( m_pAllocArray[uLoop] != uHeadLocation && uConsectutiveBlocks > 0 )
		{
			break;
		}
	}

	//This is a catch all case where if pointer is not a head, treat it as a deallocated block.
	if ( uConsectutiveBlocks == 0 )
	{
		//Set heap error.
		m_eLastError = EHeapError_Dealloc_AlreadyDeallocated;
	}
	else
	{

#ifdef _DEBUG
		//overrun/underrun checks
		if ( WasMemoryOverrun( puHeadLocation, uConsectutiveBlocks ) )
		{
			//Set heap error.
			eErrorValue = EHeapError_Dealloc_OverwriteOverrun;
		}
		else if ( WasMemoryUnderrun( puHeadLocation ) )
		{
			//Set heap error.
			eErrorValue = EHeapError_Dealloc_OverwriteUnderrun;
		}

		//Clear the allocated memory with std::fill.
		std::fill( puHeadLocation, puHeadLocation + (uConsectutiveBlocks * m_uBlockSizeInBytes), NULL );
#endif

		//decrement alloc count.
		m_uAllocCount--;

		//Set heap error to whatever value it was set to.
		m_eLastError = eErrorValue;

		//how to handle 0-size deallocations? - for now just let them act like normal allocations.
		//n.b they should just result in a header and footer (debug only) occupying a single block of memory.
	}
}


//////////////////////////////////////////////////////////////////////////
// this should return the current number of allocations
//////////////////////////////////////////////////////////////////////////
u32 CHeap::GetNumAllocs( void )
{
	return m_uAllocCount;
}


//////////////////////////////////////////////////////////////////////////
// N.B. this should return ( size of heap - size of memory in all 
// allocated blocks including overheads ).
// This value will always be slightly greater than the maximum 
// allocatable memory.
//////////////////////////////////////////////////////////////////////////
u32 CHeap::GetFreeMemory( void )
{
	return ( m_uMemorySizeInBytes - (GetNumberOfOccupiedBlocks() * m_uBlockSizeInBytes) );
}


//////////////////////////////////////////////////////////////////////////
// Error state is set by the various functions in CHeap. 
// This function just returns the most recent. No error stack.
//////////////////////////////////////////////////////////////////////////
CHeap::EHeapError CHeap::GetLastError( void )
{
	return m_eLastError;
}


//////////////////////////////////////////////////////////////////////////
// returns 0 or a +ve value to add to the address of pPointerToAlign
// to align it to uAlignment.
// 
// Example of usage to get a 64 byte aligned address:
// 
// void*	pUnalignedAddress		= GetMemoryFromSomewhere();
// u32		uAlignmentDelta			= CHeap::CalculateAlignmentDelta( pUnaligned, 64 );
// u8*		p64ByteAlignedAddress	= ((u8*) pUnaligned ) + uAlignmentDelta );
//
// N.B. uAlignment MUST be a power of 2, this function does NOT check this
// you will have to check that in your code before calling this function
//////////////////////////////////////////////////////////////////////////
// static
u32 CHeap::CalculateAlignmentDelta( void* pPointerToAlign, u32 uAlignment )
{
    // Power of two numbers have lots of almost magical seeming properties in binary.
    //
    // I leave it to you to work out how this code aligns uAligned to uAlignment...
    //
    // Hint: write out the binary values long hand at each step in the calculation
    //
    // Hint 2: you can use very similar techniques to test whether a 
    // binary number is a power of 2 (which might come in handy in passing one of the other tests...)
	u32 uAlignAdd		= uAlignment - 1;
	u32 uAlignMask		= ~uAlignAdd;
	u32 uAddressToAlign	= ((u32) pPointerToAlign); 
	u32 uAligned		= uAddressToAlign + uAlignAdd;
	uAligned			&= uAlignMask;
	return( uAligned - uAddressToAlign );
}

bool CHeap::IsSystemLittleEndian()
{
	short int iNumber = 0x1;
	char* pcNumPtr = ( char* ) &iNumber;
	return ( pcNumPtr[0] == 1 );
}

u32* CHeap::GetFreeBlockLocation( u32 uNumBytes )
{
	u32* puRtnAddress = nullptr;

	switch ( m_eAllocPolicy )
	{
		//FIRSTAVAIL - get the first encountered fitting block.
		case FirstAvail:
		{
			puRtnAddress = FirstAvailBlock( uNumBytes );
			break;
		}
		//BESTFIT - Try to allocate to the smallest available fitting block.
		case BestFit:
		{
			//Implementation Postponed.
			break;
		}
		//WORSTFIT - Try to allocate to the largest available fitting block.
		case WorstFit:
		{
			//Implementation Postponed.
			break;
		}
	}

	//pRtnAddress is only null if no consecutive subset of 'free' blocks could be allocated.
	//Set error.
	if ( puRtnAddress == nullptr )
	{
		m_eLastError = EHeapError_Alloc_NoLargeEnoughBlocks;
	}

	//Should no free block be available, return nullptr.
	return puRtnAddress;
}

u32 CHeap::GetArrayIndexFromArrayPtr( u32* puArrayPtr )
{
	return ( puArrayPtr - m_pAllocArray );
}

u32* CHeap::GetBlockHeadFromArray(u32 * pArrayPtr)
{
	u32 uPositionOffset = (GetArrayIndexFromArrayPtr(pArrayPtr) * m_uBlockSizeInBytes);
	u32 uMemoryPosition = ((u32)(m_pBaseAllocatableAddress)) + uPositionOffset;
	return ( (u32*) uMemoryPosition );
}

u32 CHeap::GetNumberOfOccupiedBlocks()
{
	u32 uReturnValue = 0;
	for (unsigned int uLoop = 0; uLoop < m_uMaxAllocs; ++uLoop)
	{
		if ( m_pAllocArray[uLoop] != NULL )
		{
			uReturnValue++;
		}
	}

	return uReturnValue;
}

u32* CHeap::FirstAvailBlock( u32 uNumBytes )
{
	u32* puRtnAddress = nullptr;

	//The pointer that points to the first 'free' array entry.
	u32* puFreeBlockHead = nullptr;
	u32 uConsectutiveFree = 0;
	u32 uStartingIndex = 0;

	//Iterate through the array.
	for ( unsigned int uLoop = 0; uLoop < m_uMaxAllocs; ++uLoop )
	{
		//1) Iterate through blocks until the first 'free' block is encountered.
		if ( puFreeBlockHead == nullptr && m_pAllocArray[uLoop] == NULL )
		{
			puFreeBlockHead = &( m_pAllocArray[uLoop] );
			uStartingIndex = uLoop;
			uConsectutiveFree++;
		}
		//2) Count consecutive 'free' blocks and compare to uNumBytes
		else if ( puFreeBlockHead != nullptr && m_pAllocArray[uLoop] == NULL )
		{
			uConsectutiveFree++;
		}
		//3) Should an 'occupied' block be encountered, restart from 1).
		else if ( puFreeBlockHead != nullptr && m_pAllocArray[uLoop] != NULL )
		{
			puFreeBlockHead = nullptr;
			uStartingIndex = 0;
			uConsectutiveFree = 0;
		}

		//4) If GTE accept set pRtnAddress to pFreeBlockHead, else continue.
		if ( uNumBytes <= ( m_uBlockSizeInBytes * uConsectutiveFree ) )
		{
			//Get the actual block head location from the array.
			puRtnAddress = GetBlockHeadFromArray(puFreeBlockHead);

			//Set array data to pFreeBlockHead
			for ( unsigned int uLoop2 = uStartingIndex; uLoop2 <= uLoop; ++uLoop2 )
			{
				m_pAllocArray[uLoop2] = ( u32 ) puRtnAddress;
			}
			break;
		}
	}

	//Should no free block be available, return nullptr.
	return puRtnAddress;
}

u32* CHeap::BestFitBlock( u32 uNumBytes )
{
	return nullptr;
}

u32* CHeap::WorstFitBlock( u32 uNumBytes )
{
	return nullptr;
}

bool CHeap::WasMemoryUnderrun( u8* pHeadLocation )
{
	u32 puHeadVal = 0;
	bool bReturnValue = false;

	//for each iteration of 0xFACCFACC guard (4207737548), check value.
	for ( u32 uLoop = 0; uLoop < sm_kuNumberOfHeaderSequences; ++uLoop ) 
	{
		puHeadVal = *( (u32*) pHeadLocation + uLoop );
		if ( puHeadVal != sm_kuConsistencyValue ) 
		{
			bReturnValue = true;
		}
	}

	return bReturnValue;
}

bool CHeap::WasMemoryOverrun( u8* pHeadLocation, u32 uNumberOfBlocks )
{
	//Get the pointer to the head of the block after
	u8* pEndByte = pHeadLocation + (m_uBlockSizeInBytes * ( uNumberOfBlocks ));
	u8* pStartByte = 0;

	//If the check limit is in the same block, set it to the theoretical footer limit
	if ( ( pEndByte - pHeadLocation ) <= m_uBlockSizeInBytes )
	{
		pStartByte = pHeadLocation + ( sm_kuNumberOfHeaderSequences * sizeof( sm_kuConsistencyValue ) );
	}
	//Else check into the previous block aka at least one byte of the footer sequence must be in the last block.
	else 
	{
		pStartByte = pEndByte - ( m_uBlockSizeInBytes + (( sizeof( sm_kuConsistencyValue ) * sm_kuNumberOfHeaderSequences) - 1));
	}

	//Get a byte array of the sequence to check against.
	unsigned char bytes[sizeof(sm_kuConsistencyValue)];
	std::copy( reinterpret_cast<const unsigned char*>( &sm_kuConsistencyValue ),
		reinterpret_cast<const unsigned char*>( &sm_kuConsistencyValue ) + sizeof( sm_kuConsistencyValue ),
		bytes );

	//Iterate backwards byte by byte until the entire footer sequence is matched or reached the start of the block.
	//N.B. If part of the sequence is matched at the start of the last block, continue iterating backwards until match or break.

	u8* puIterator = pStartByte;
	for ( u32 uLoop = 0; uLoop < sm_kuNumberOfHeaderSequences; ++uLoop ) 
	{
		if ( uLoop > 0 ) 
		{
			puIterator += sizeof( sm_kuConsistencyValue );
		}
		puIterator = std::search( puIterator, pEndByte, std::begin( bytes ), std::end( bytes ) );
	}

	//If the iterator reaches the end of the allocated memory, that means a match was not found (no footer sequence was encountered)
	return ( puIterator == pEndByte );
}
