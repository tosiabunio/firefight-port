#ifndef STRICT
#define STRICT
#endif

#pragma warning(disable : 4127 4201 4512 4514 4710)

#ifdef _DEBUG
#pragma warning(4 : 4702 )
#else
#pragma warning(disable : 4702)
#endif