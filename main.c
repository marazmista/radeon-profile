#include "radeon_ioctl.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CARD_SIZE 10
#define TRIM(string) strtok(string,"\n")

int main(int argc, char *argv[]){
    char card[CARD_SIZE];
    struct buffer b;
    int fd;

    if(getuid())
        puts("WARNING: access to radeon ioctl requires root privileges");

    if(argc == 2)
        strcpy(card, argv[1]);
    else {
        printf("Which card? ");
        fgets(card, CARD_SIZE, stdin);
        TRIM(card);
    }

    if(!strlen(card)){
        puts("Nothing inserted, exit");
        return -1;
    }

    fd = openCardFD(card);
    if(fd < 0){
        printf("Open of %s %s failed, exit\n", argv[1], argv[2]);
        return -2;
    }

    b=radeonTemperature(fd);
    if(!b.error)
        printf("Temperature: %3.1f °C \n", (float)b.value.milliCelsius/1000);

    b=radeonCoreClock(fd);
    if(!b.error)
        printf("Core clock: %u MHz \n", b.value.MegaHertz);

    b=radeonMemoryClock(fd);
    if(!b.error)
        printf("Memory clock: %u MHz \n", b.value.MegaHertz);

    b=radeonGttUsage(fd);
    if(!b.error)
        printf("Used GTT: %lu KB \n", b.value.byte/1024);

    b=radeonVramUsage(fd);
    if(!b.error)
        printf("Used VRAM: %lu KB \n", b.value.byte/1024);

    closeCardFD(fd);
}
