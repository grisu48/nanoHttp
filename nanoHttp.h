#ifndef _NANOHTTP_H_
#define _NANOHTTP_H_

#include <string.h>



/* ==== GLOBAL DEFINES ====================================================== */
// Multi-threading on
#define _NANOHTTP_THREADING 1

// Maximum number of threads
#if _NANOHTTP_THREADING == 1
	#define THREAD_COUNT 10
#endif

// String buffer size
#define STR_BUF  4*1024
#define READ_BUF 4*1024

#endif



/* Function prototypes */
static char *get_mime_type(char *name);

