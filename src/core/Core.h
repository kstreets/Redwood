#pragma once

#ifdef RWD_BUILD_DLL
	#define	RWD_API __declspec(dllexport)
#else
	#define	RWD_API __declspec(dllimport)
#endif
