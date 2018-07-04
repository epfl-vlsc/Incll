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
#define GLOBAL_FLUSH

//default 15
#define KEY_LW 12
