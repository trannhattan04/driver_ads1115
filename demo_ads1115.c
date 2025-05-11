#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h> // Include errno header

#define DEVICE_PATH "/dev/ads1115"
#define TYPE 'a'
#define READ_AIN0_ADS1115 _IOR(TYPE, 1, int)
#define READ_AIN1_ADS1115 _IOR(TYPE, 2, int)
#define READ_AIN2_ADS1115 _IOR(TYPE, 3, int)
#define READ_AIN3_ADS1115 _IOR(TYPE, 4, int)

int main(){
    int fd;
    int data;

    // Open the device
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open the device");
        return errno;
    }

    // Read AIN0 data
    if (ioctl(fd, READ_AIN0_ADS1115, &data) < 0) {
        perror("Failed to read AIN0 data");
        close(fd);
         return errno;
    }
    printf("GIÁ TRỊ ĐIỆN ÁP AIN0 (mV) la: %d\n", data);

    // Read AIN1 data
    if (ioctl(fd, READ_AIN1_ADS1115, &data) < 0) {
        perror("Failed to read AIN1 data");
        close(fd);
        return errno;
    }
    printf("GIÁ TRỊ ĐIỆN ÁP AIN1 (mV) la: %d\n", data);

    // Read AIN2 data
    if (ioctl(fd, READ_AIN2_ADS1115, &data) < 0) {
        perror("Failed to read AIN2 data");
        close(fd);
        return errno;
    }
    printf("GIÁ TRỊ ĐIỆN ÁP AIN2 (mV) la: %d\n", data);

    // Read AIN3 data
    if (ioctl(fd, READ_AIN3_ADS1115, &data) < 0) {
        perror("Failed to read AIN3 data");
        close(fd);
        return errno;
    }
    printf("GIÁ TRỊ ĐIỆN ÁP AIN3 (mV) la: %d\n", data);


    // Close the device
    close(fd);
    return 0;
}