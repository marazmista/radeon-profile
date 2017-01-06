#include "radeon_ioctl.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CARD_SIZE 10
#define TRIM(string) strtok(string,"\n")
#define TOTAL_TIME 1000000 // 1 second
#define FREQUENCY 200 // 200 Hz

int main(int argc, char *argv[]){
    char card[CARD_SIZE];
    struct buffer b;
    int fd, loop = 0;

    if(getuid())
        puts("WARNING: access to radeon ioctl requires root privileges");

    if(argc >= 2){
        strcpy(card, argv[1]);
    } else {
        if(!fork())
            execl("/usr/bin/ls", "ls", "/dev/dri/", (char*)NULL);
        write(1, "Which card?\n", 13);
        fgets(card, CARD_SIZE, stdin);
        TRIM(card);
    }

    if(!strlen(card)){
        puts("Nothing inserted, exit");
        return -1;
    }

    fd = openCardFD(card);
    if(fd < 0){
        printf("Open of %s failed, exit\n", card);
        return -2;
    }

    loop = (argc >= 3) && !strcmp(argv[2],"--loop");
    do{
        puts("\nRADEON");
        radeonTemperature(fd, &b);
        if(!b.error)
            printf("Temperature: %3.1f °C \n", b.value.milliCelsius/1000.0f);

        radeonCoreClock(fd, &b);
        if(!b.error){
            printf("Core clock: %u MHz", b.value.MegaHertz);

            radeonMaxCoreClock(fd, &b);
            if(!b.error)
                printf(" (max: %u MHz)", b.value.KiloHertz/1000);
            putchar('\n');
        }

        radeonMemoryClock(fd, &b);
        if(!b.error)
            printf("Memory clock: %u MHz \n", b.value.MegaHertz);

        radeonGttUsage(fd, &b);
        if(!b.error)
            printf("Used GTT: %lu KB \n", b.value.byte/1024);

        radeonVramUsage(fd, &b);
        if(!b.error)
            printf("Used VRAM: %lu KB \n", b.value.byte/1024);

        radeonGpuUsage(fd, &b, TOTAL_TIME, FREQUENCY);
        if(!b.error)
            printf("Used GPU: %3.1f %% \n", b.value.percentage);

        puts("\nAMDGPU");
        amdgpuGttUsage(fd, &b);
        if(!b.error)
            printf("Used GTT: %lu KB \n", b.value.byte/1024);

        amdgpuVramUsage(fd, &b);
        if(!b.error){
            printf("Used VRAM: %lu KB", b.value.byte/1024);

            amdgpuVramSize(fd, &b);
            if(!b.error)
                printf(" (total: %lu KB)", b.value.byte/1024);
            putchar('\n');
        }

        amdgpuGpuUsage(fd, &b, 1000000, 200);
        if(!b.error)
            printf("Used GPU: %3.1f %% \n", b.value.percentage);
    } while(loop);

    closeCardFD(fd);
}
