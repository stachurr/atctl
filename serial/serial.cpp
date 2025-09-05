#include "serial.h"
#include "../source_exception/source_exception.h"
#include "../common.h"

#include <fcntl.h>
#ifndef _WIN32
    #include <unistd.h>
#endif



#ifdef _WIN32
static void _windows_perror (const char *funcname, size_t depth)
{
    constexpr size_t MAX_ERROR_RECURSION_DEPTH = 2;

    constexpr DWORD MAX_TEXT_WIDTH  = 0; // 0 => no width restrictions
    constexpr DWORD DEFAULT_LANG    = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    constexpr DWORD FMT_FLAGS       = FORMAT_MESSAGE_ALLOCATE_BUFFER    // -> use 0 as output buffer size.
                                    | FORMAT_MESSAGE_FROM_SYSTEM
                                    | (MAX_TEXT_WIDTH & FORMAT_MESSAGE_MAX_WIDTH_MASK);


    if (depth < MAX_ERROR_RECURSION_DEPTH)
    {
        const DWORD err = GetLastError();
        LPSTR msg = nullptr;

        if (0 == FormatMessageA(FMT_FLAGS, nullptr, err, DEFAULT_LANG, (LPSTR)&msg, 0, nullptr))
        {
            _windows_perror("FormatMessageA", depth + 1);
        }
        else if (msg)
        {
            fprintf(stderr, "%s: (%lu) %s", funcname, err, msg);
            LocalFree(msg);
        }
        else
        {
            fprintf(stderr, "windows_perror: Unexpected error while formatting error message!\n");
        }
    }
    else
    {
        fprintf(stderr, "windows_perror: Maximum error recursion depth reached (%u). Too many nested errors.\n", MAX_ERROR_RECURSION_DEPTH);
    }
}

void windows_perror (const char *funcname)
{
    _windows_perror(funcname, 0);
}
#endif



#ifdef _WIN32
ssize_t win32_serial_device::read (void *buffer, size_t size)
{
    DBG("Reading (requesting %lu bytes)...\n", size);
    ssize_t n_read = 0;
    COMSTAT stat;

    if (ClearCommError(this->get_handle(), NULL, &stat))
    {
        if (stat.cbInQue < size)
        {
            size = stat.cbInQue;
            DBG("Request updated to %lu bytes\n", size);
        }

        if (!ReadFile(this->get_handle(), buffer, size, &reinterpret_cast<DWORD&>(n_read), NULL))
        {
            windows_perror("ReadFile");
            n_read = -1;
        }
    }
    else
    {
        windows_perror("ClearCommError");
        n_read = -1;
    }


    DBG("Reading (received %ld bytes)...\n", n_read);
    return n_read;
}

ssize_t win32_serial_device::write (const void *buffer, size_t size)
{
    DBG("Writing (sending %lu bytes)...\n", size);
    ssize_t n_written = 0;

    if (!WriteFile(this->get_handle(), buffer, size, &reinterpret_cast<DWORD&>(n_written), NULL))
    {
        windows_perror("WriteFile");
        n_written = -1;
    }

    DBG("Writing (sent %ld bytes)...\n", n_written);
    return n_written;
}

HANDLE win32_serial_device::open_handle (const char *device)
{
    DBG("Opening device...\n");

    if (!device)
    {
        throw source_exception("invalid argument");
    }

    HANDLE handle = CreateFileA(device,
                                GENERIC_READ | GENERIC_WRITE,
                                0,      // disallow sharing
                                NULL,   // no security attr.
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);  // "When opening an existing file, CreateFile ignores this parameter."

    if (INVALID_HANDLE_VALUE == handle)
    {
        if (ERROR_FILE_NOT_FOUND == GetLastError())
        {
            throw source_exception("Serial device does not exist");
        }

        windows_perror("CreateFileA");
    }

    return handle;
}

bool win32_serial_device::close_handle (HANDLE handle)
{
    DBG("Closing device...\n");
    return CloseHandle(handle);
}

int win32_serial_device::wait_for_data (size_t timeout_ms)
{
    return 1;
    int status;

    switch (WaitForSingleObject(this->get_handle(), timeout_ms))
    {
        case WAIT_OBJECT_0:
            status = 1;
            break;

        case WAIT_TIMEOUT:
            status = 0;
            break;

        default:
        case WAIT_ABANDONED:
        case WAIT_FAILED:
            status = -1;
            windows_perror("WaitForSingleObject");
            break;
    }

    return status;
}
#else
ssize_t posix_serial_device::read (void *buffer, size_t size)
{
    const ssize_t status = ::read(this->get_handle(), buffer, size);
    if (-1 == status)
    {
        perror("read");
    }
    return status;
}

ssize_t posix_serial_device::write (const void *buffer, size_t size)
{
    const ssize_t status = ::write(this->get_handle(), buffer, size);
    if (-1 == status)
    {
        perror("write");
    }
    return status;
}

int posix_serial_device::open_handle (const char *device)
{
    if (!device)
    {
        throw source_exception("invalid argument");
    }

    const std::filesystem::path path(device);

    if (!std::filesystem::exists(path))
    {
        throw source_exception("does not exist");
    }

    if (!std::filesystem::is_character_file(path))
    {
        throw source_exception("not a serial device");
    }

    return ::open(device, O_RDWR);
}

bool posix_serial_device::close_handle (int fd)
{
    return ::close(fd);
}

int posix_serial_device::wait_for_data (size_t timeout_ms)
{
    fd_set rfds;
    struct timeval timeout;
    struct timeval timeout_og;
    int status;


    if (timeout_ms > 999) {
        timeout_og.tv_sec = timeout_ms / 1000;
        timeout_og.tv_usec = (timeout_ms % 1000) * 1000;
    }
    else {
        timeout_og.tv_sec = 0;
        timeout_og.tv_usec = timeout_ms * 1000;
    }


    bool continue_waiting = true;
    while (continue_waiting)
    {
        FD_ZERO(&rfds);
        FD_SET(this->get_handle(), &rfds);
        timeout = timeout_og;

        continue_waiting = false;
        status = select(this->get_handle() + 1, &rfds, NULL, NULL, &timeout);

        if (status == -1)
        {
            if (EINTR == errno)
            {
                continue_waiting = true;
            }
            else
            {
                perror("select");
            }
        }
    }

    return status;
}
#endif
