#pragma once

#include "basic_serial_device.h"

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
#endif



#ifdef _WIN32
class win32_serial_device : public basic_serial_device<HANDLE, INVALID_HANDLE_VALUE>
{
public:
    ssize_t read (void *buffer, size_t size) override;
    ssize_t write (const void *buffer, size_t size) override;
    int wait_for_data (size_t timeout_ms) override;

private:
    HANDLE open_handle (const char *device) override;
    bool close_handle (HANDLE handle) override;
};
#else
class posix_serial_device : public basic_serial_device<int, -1>
{
public:
    ssize_t read (void *buffer, size_t size) override;
    ssize_t write (const void *buffer, size_t size) override;
    int wait_for_data (size_t timeout_ms) override;

private:
    int open_handle (const char *device) override;
    bool close_handle (int handle) override;
};
#endif



#ifdef _WIN32
using serial_device = win32_serial_device;
void windows_perror (const char *funcname);
#else
using serial_device = posix_serial_device;
#endif
