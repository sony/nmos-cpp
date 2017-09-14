#pragma warning( disable : 4290 ) // C++ Exception Specification ignored
//#pragma warning( disable : 4097 ) // typedef-name used as synonym for class-name
//#pragma warning( disable : 4514 ) // unreferenced inline function has been removed
#pragma warning( disable : 4786 ) // identifier was truncated to '255' characters in the browser information
#pragma warning( disable : 4100 ) // unreferenced formal parameter
#pragma warning( disable : 4512 ) // assignment operator could not be generated

#pragma warning( disable : 4503 ) // decorated name length exceeded, name was truncated
#pragma warning( disable : 4510 ) // default constructor could not be generated
#pragma warning( disable : 4610 ) // object can never be instantiated - user defined constructor required

// New for VC8 (Visual Studio 2005)

#pragma warning( disable : 4344 ) // behavior change: use of explicit template arguments
#pragma warning( disable : 4351 ) // new behavior : elements of array will be default initialized

// Disable warnings for perfectly standard C++ Standard Library and POSIX functions;
// from VC90 (Visual Studio 2008) even Microsoft noted that "in fact, these functions
// are NOT deprecated from the standard" and changed their preprocessor definitions from
// xxx_NO_DEPRECATE to xxx_NO_WARNINGS (though the VC8 names continue to work).

//#pragma warning( disable : 4996 ) // 'function' was declared deprecated
#ifndef _SCL_SECURE_NO_DEPRECATE // 'function': Function call with parameters that may be unsafe
#define _SCL_SECURE_NO_DEPRECATE
#endif
#ifndef _CRT_SECURE_NO_DEPRECATE // 'function'. This function or variable may be unsafe.
#define _CRT_SECURE_NO_DEPRECATE
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE // 'function'. The POSIX name for this item is deprecated.
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

// New for VC14 (Visual Studio 2015)

#pragma warning( disable : 4456 ) // declaration of 'variable' hides previous local declaration
#pragma warning( disable : 4458 ) // declaration of 'variable' hides class member
#pragma warning( disable : 4459 ) // declaration of 'variable' hides global declaration
#pragma warning( disable : 4592 ) // symbol will be dynamically initialized (implementation limitation)
