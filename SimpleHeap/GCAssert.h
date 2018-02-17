#ifndef _GCASSERT_H_
#define _GCASSERT_H_

#if defined(_DEBUG)

	//////////////////////////////////////////////////////////////////////////
	// assert macro - kills the app when BooleanExpression != true
	//
	// example usage:
	// GCAssert( 0 == iThing, "iThing should not be zero" );
	// GCAssert( iThing < 69, "iThing must be < 69 (current value: %d)", iThing );
	//////////////////////////////////////////////////////////////////////////
#define GCASSERT( BooleanExpression, FormatString, ... )	if(!(BooleanExpression)){_GCAssert::_InternalAssertFail( __FUNCSIG__, __LINE__, __FILE__, FormatString, __VA_ARGS__ );__asm{ int 3 }}

	//////////////////////////////////////////////////////////////////////////
	// helper for assert code in cases where if you need a local variable to 
	// make the assert message helpful.
	//
	// e.g.
	// GCASSERT_ONLY( ENetworkError eNetError = CNetworkManager::GetLastError() );
	// GCASSERT( ENetErr_OK == eError, "Network error! %s", CNetworkManager::ErrorToString( eNetError ) ); 
	//////////////////////////////////////////////////////////////////////////
	#define GCASSERT_ONLY( code )	code


#else

	// both boil away in non debug builds
	#define GCASSERT( BooleanExpression, FormatString, ... )	/*nothing*/
	#define GCASSERT_ONLY( code )								/*nothing*/


#endif //#if defined(_DEBUG) ... #else ...


namespace _GCAssert
{
	void _InternalAssertFail( const char* pstrFunctionSig, int iLineNumber, const char* pstrFileName, const char* pstrFormat, ... );
};

#endif // #ifndef _GCASSERT_H_
