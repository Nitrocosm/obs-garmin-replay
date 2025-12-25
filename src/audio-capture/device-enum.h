#ifndef DEVICE_ENUM_H
#define DEVICE_ENUM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DEVICE_ID_LEN 256
#define MAX_DEVICE_NAME_LEN 256
#define MAX_DEVICES 32

// Device information
typedef struct device_info {
    char id[MAX_DEVICE_ID_LEN];
    char name[MAX_DEVICE_NAME_LEN];
} device_info_t;

// List of devices
typedef struct device_list {
    int count;
    device_info_t devices[MAX_DEVICES];
} device_list_t;

// Enumerate available microphones
// Returns a device list that must be freed with device_list_free()
device_list_t *device_enum_microphones(void);

// Free a device list
void device_list_free(device_list_t *list);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_ENUM_H
