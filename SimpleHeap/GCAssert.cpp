#include "stdafx.h"

#include "windows.h"

#include "GCAssert.h"

namespace _GCAssert
{
	// constants used to pass text to the assert dialog
	const size_t k_AssertBufferSize_UserSprintf		= 256;
	const size_t k_AssertBufferSize_DialogMessage	= 512;


	// assert function
	void _InternalAssertFail(	const char* pstrFunctionSig, 
								int			iLineNumber, 
								const char* pstrFileName, 
								const char* pstrFormat, ... )
	{
		// insane workaround to allow passing of var args from the macro into sprintf - note use of vsprintf
		char achUserFormat[ k_AssertBufferSize_UserSprintf ];
		va_list vaExtraArgs;
		va_start( vaExtraArgs, pstrFormat );
		vsprintf_s( achUserFormat, k_AssertBufferSize_UserSprintf, pstrFormat, vaExtraArgs );
		va_end( vaExtraArgs );

		// now print it all into the messagebox and the DebugOut
		char	achTemp[ k_AssertBufferSize_DialogMessage ];
		WCHAR	awcTemp[ k_AssertBufferSize_DialogMessage ];
		sprintf_s( achTemp, k_AssertBufferSize_DialogMessage, "%s\n\nfn: %s\n\n[file: %s (line: %d)]", achUserFormat, pstrFunctionSig, pstrFileName, iLineNumber ); 
		size_t uNumCharsVConverted = 0;
		mbstowcs_s( &uNumCharsVConverted, awcTemp, k_AssertBufferSize_DialogMessage, achTemp, k_AssertBufferSize_DialogMessage );
		OutputDebugString( (LPCWSTR)L"######################################################################################\n"
									L"GCAssert Has Fired\n" ); 			
		OutputDebugString( awcTemp ); 			
		OutputDebugString( (LPCWSTR)L"\n######################################################################################\n" );
		MessageBox(
			NULL,
			awcTemp,
			((LPCWSTR)L"GCAssert Has Fired"),
			MB_ICONSTOP | MB_OK
		);			
	}
};
