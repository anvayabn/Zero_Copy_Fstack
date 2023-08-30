/*
 * Some sort of Copyright
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR(fmt, ...) ({						\
	fprintf(stderr, fmt "\n", ##__VA_ARGS__);			\
	exit(EXIT_FAILURE);						\
})

#define SYSERROR(fmt, ...) ({						\
	fprintf(stderr, fmt ": %s\n", ##__VA_ARGS__, strerror(errno));	\
	exit(EXIT_FAILURE);						\
})
