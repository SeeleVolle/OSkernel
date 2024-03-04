#define SYS_WRITE   64
#define SYS_GETPID  172
#include "stddef.h"
int sys_write(unsigned int fd, const char* buf, size_t count);

int sys_getpid(void);