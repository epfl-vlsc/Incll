/*
 * Parameters of the system
 * Enable/disable #defines
 */

#pragma once

//DBGLOG
#define DBGLOG(text) \
	printf("%s at %s:%d\n", text, __FILE__, __LINE__);

#define DBGLOGPTR(text, ptr) \
	printf("%s at %p at %s:%d\n", text, ptr, __FILE__, __LINE__);


//default epoch 16
#define GL_FREQ 16

//periodic flush
//#define GLOBAL_FLUSH

//in-cache line log
//#define INCLL
#ifdef INCLL
	#define LN_ATTR
	#define LN_SAVE
	#define LN_LOAD

	#define IN_ATTR
	#define IN_SAVE
	#define IN_LOAD
#endif

//nvm log
//#define NVMLOG

//persistent allocator
//#define PALLOC


//deafult key 15
#define KEY_LW 12
#define KEY_IW 11
#define KEY_MID KEY_LW/2
