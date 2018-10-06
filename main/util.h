#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef swp
#define swp(_a, _b) \
	do { \
		typeof((_a)) tmp = (_a); \
		(_a) = (_b); \
		(_b) = tmp; \
	} while(0)
#endif

#define ARRAY_REVERSE(_arr, _len) \
	do { \
		typeof((_len)) i; \
		for(i = 0; i < _len / 2; i++) swp(((_arr)[i]), ((_arr)[_len - i - 1])); \
	} while(0)

						
#define ARRAY_LEN(_arr) (sizeof((_arr)) / sizeof(*(_arr)))

#define DIRENT_FOR_EACH(cursor, dir) \
	for((cursor) = readdir((dir)); (cursor); (cursor) = readdir((dir)))
