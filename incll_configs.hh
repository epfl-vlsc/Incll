/*
 * Parameters of the system
 * Enable/disable #defines
 */

#pragma once

//Debug Logging
//#define DBG
#ifdef DBG
#define DBGLOG(f_, ...) \
		printf(("At %s:%d " f_ "\n"), __FILE__, __LINE__, ##__VA_ARGS__);
#else
#define DBGLOG(f_, ...)
#endif

//default epoch 16
#define GL_FREQ 16

//periodic flush
#define GLOBAL_FLUSH

//default 15
#define KEY_LW 12

//disable dealloc for remove
//#define DISABLE_DEALLOC

// in-cacheline log features
#define INCLL
#ifdef INCLL
#define Ifincll(statement) statement
#else //incll
#define Ifincll(statement)
#endif //incll
