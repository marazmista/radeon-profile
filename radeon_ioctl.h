#ifndef RADEON_IOCTL_H
#define RADEON_IOCTL_H



int openCardFD(char const * const card);
void closeCardFD(int const fd);

struct buffer {
    int error; // Data is valid only if error == 0
    union {
        unsigned int MegaHertz;
        unsigned int KiloHertz;
        int milliCelsius;
        unsigned long byte;
    } value;
};

void radeonTemperature(int const fd, struct buffer *data); // milliCelsius
void radeonCoreClock(int const fd, struct buffer *data); // MegaHertz
void radeonMemoryClock(int const fd, struct buffer *data); // MegaHertz
void radeonMaxCoreClock(int const fd, struct buffer *data); // KiloHertz
void radeonVramUsage(int const fd, struct buffer *data); // byte
void radeonGttUsage(int const fd, struct buffer *data); // byte
void amdgpuVramSize(int const fd, struct buffer *data); // byte
void amdgpuVramUsage(int const fd, struct buffer *data); // byte
void amdgpuGttUsage(int const fd, struct buffer *data); // byte



#endif
