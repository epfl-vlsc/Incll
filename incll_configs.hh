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

//periodic flush
#define GLOBAL_FLUSH

//default 15
#define KEY_LW 14
#define KEY_MID KEY_LW/2

//disable dealloc for remove
//#define DISABLE_DEALLOC

// in-cacheline log features
#define INCLL
#ifdef INCLL
#define Ifincll(statement) statement;
#else //incll
#define Ifincll(statement)
#endif //incll

#define EXTLOG
#ifdef EXTLOG
#define LN_EXTLOG
#define LN_EXTLOG_INCLL
#define IN_EXTLOG
#endif //extlog

#define YCSB

//useful for checking recovery
#define USE_DEV_SHM

//stats and recovery
//#define MTAN
//#define YCSB_RECOVERY
//#define EXTLOG_STATS
