#define _GNU_SOURCE
#include<stdint.h>
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BLOCK_SIZE (1 << 12)
char *filename = "/home/sin/test.txt";

static inline unsigned long long rdtscp(void)
{
  unsigned long long lo, hi;
  __asm__ __volatile__(
      "rdtscp"
      : "=a"(lo), "=d"(hi));
  return (unsigned long long)hi << 32 | lo;
}

int main(int argc, char **argv){
    int ret;
    int fd = open(filename, O_RDWR | O_SYNC | O_DIRECT);
    uint64_t begin, end;
    char *buf = NULL;

    if (fd < 0){
        printf("File open error!\n");
        return -1;
    }

    struct stat st;

    if (fstat(fd, &st) == -1)
    {
        printf("Stat file error!\n");
        close(fd);
    }

    size_t size = st.st_size + (getpagesize() - st.st_size % getpagesize());
    posix_memalign((void **)&buf, getpagesize(), size);

    if(buf == NULL)
        printf("Malloc buffer failed!\n");

    begin = rdtscp();
    ret = read(fd, buf, size);
    end = rdtscp();
    
    if (ret != st.st_size)
        printf("Read %d, Size:%lu\n", ret, st.st_size);

    printf("[%lu:%lu] Cycles: %lu\n", begin, end, end - begin);

    close(fd);

    return 0;
}
