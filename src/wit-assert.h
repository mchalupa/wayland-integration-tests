#ifndef __WIT_ASSERT_H__
#define __WIT_ASSERT_H__

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>

/* Assert with formated output */
#define assertf(cond, ...) 							\
	do {									\
		if (!(cond)) {							\
			fprintf(stderr, "%s (%s: %d): Assertion %s failed!",	\
					__FUNCTION__, __FILE__, __LINE__, #cond);\
			fprintf(stderr, " " __VA_ARGS__);			\
			putc('\n', stderr);					\
			abort();						\
		}								\
	} while (0)

#define dbg(...) 								\
	do {									\
		fprintf(stderr, "{{%6d | %s in %s: %d: | ", getpid(),		\
				__FUNCTION__, __FILE__, __LINE__);		\
		fprintf(stderr,	__VA_ARGS__);					\
	} while (0)

#endif /* __WIT_ASSERT_H__ */
