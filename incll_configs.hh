/*
 * Parameters of the system
 * Enable/disable #defines
 */

#pragma once

//Debug Logging
//#define DBG
#ifdef DBG
#define REC_ASSERT(cond) assert(cond);
#define DBGLOG(f_, ...) \
		printf(("At %s:%d " f_ "\n"), __FILE__, __LINE__, ##__VA_ARGS__);
#else
#define REC_ASSERT(cond)
#define DBGLOG(f_, ...)
#endif

//------------------------------------------------------------------------------------
//default epoch 16
#define GL_FREQ 16
#define YCSB



//all defines
//#define DISABLE_ALL

#ifdef DISABLE_ALL
#define KEY_LW 15
#define KEY_MID KEY_LW/2
#else //disable all

#define KEY_LW 14
#define KEY_MID KEY_LW/2
#define GLOBAL_FLUSH
#define INCLL
#define EXTLOG
#define USE_DEV_SHM
#define PALLOCATOR
#endif //disable all


#ifdef EXTLOG
#define LN_EXTLOG
#define LN_EXTLOG_INCLL
#define IN_EXTLOG
#endif //extlog

//#define PERF_WORKLOAD
#define REMOVE_HEAP

//disable dealloc for remove
//#define DISABLE_DEALLOC

//stats and recovery
//#define MTAN
//#define YCSB_RECOVERY
//#define EXTLOG_STATS
