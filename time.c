#include <stdio.h>
#include <sys/time.h>

int main (int argc, char *argv[])
{
        struct timeval t, old, delta;

        while (1) {
                usleep(4000);
                gettimeofday(&t, NULL);
                if (old.tv_sec > t.tv_sec) {
                        delta.tv_sec = t.tv_sec - old.tv_sec;
                        delta.tv_usec = t.tv_usec - old.tv_usec;
                        printf("Delta time: %ld %ld\n", delta.tv_sec,
                                delta.tv_usec);
                	printf("%ld %ld\n", t.tv_sec, t.tv_usec);
                	printf("%ld %ld\n", old.tv_sec, old.tv_usec);
                }
                old = t;
        }
        return 0;
}


