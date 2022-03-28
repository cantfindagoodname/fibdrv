#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"


static inline unsigned long long fls(unsigned long long);
unsigned long long i_sqrt(unsigned long long);
unsigned long long getmean64(unsigned long long *);

int main()
{
    FILE *fp = fopen("time.txt", "w");

    int offset = 500;
    uint32_t buf[256];

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    fprintf(fp, "n ktime(ns)\n");
    unsigned long long ksz_arr[64];
    for (int i = 0; i <= offset; i++) {
        for (int index = 0; index < 64; index++) {
            lseek(fd, i, SEEK_SET);
            read(fd, buf, 256);
            ksz_arr[index] = write(fd, NULL, 0);
        }
        unsigned long long ksz = getmean64(ksz_arr);
        fprintf(fp, "%d %llu\n", i, ksz);
    }

    fclose(fp);
    close(fd);
    return 0;
}

static inline unsigned long long fls(unsigned long long word)
{
    int num = 64 - 1;

    if (!(word & (~0ul << 32))) {
        num -= 32;
        word <<= 32;
    }
    if (!(word & (~0ul << (64 - 16)))) {
        num -= 16;
        word <<= 16;
    }
    if (!(word & (~0ul << (64 - 8)))) {
        num -= 8;
        word <<= 8;
    }
    if (!(word & (~0ul << (64 - 4)))) {
        num -= 4;
        word <<= 4;
    }
    if (!(word & (~0ul << (64 - 2)))) {
        num -= 2;
        word <<= 2;
    }
    if (!(word & (~0ul << (64 - 1))))
        num -= 1;
    return num;
}

unsigned long long i_sqrt(unsigned long long x)
{
    unsigned long long m, y = 0;

    if (x <= 1)
        return x;

    m = 1UL << (fls(x) & ~1UL);
    while (m) {
        unsigned long long b = y + m;
        y >>= 1;

        if (x >= b) {
            x -= b;
            y += m;
        }
        m >>= 2;
    }

    return y;
}

unsigned long long getmean64(unsigned long long *ksz)
{
    unsigned long long sum = 0;
    for (int i = 0; i < 64; ++i)
        sum += ksz[i];
    unsigned long long mean = sum >> 6;
    sum = 0;
    for (int i = 0; i < 64; ++i) {
        unsigned long long sqr = (ksz[i] - mean) * (ksz[i] - mean);
        sum += sqr;
    }
    unsigned long long std = i_sqrt(sum >> 6);
    for (int i = 0; i < 64; ++i)
        if (ksz[i] < (mean - 2 * std) || ksz[i] > (mean + 2 * std))
            ksz[i] = mean;
    sum = 0;
    for (int i = 0; i < 64; ++i)
        sum += ksz[i];
    mean = sum >> 6;
    return mean;
}
