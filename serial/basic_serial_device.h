#pragma once

#include <cstdio>

#ifdef _WIN32
    using ssize_t = long;
#endif



template<typename HANDLE_T, HANDLE_T _INVALID_HANDLE>
class basic_serial_device {
protected:
    static constexpr HANDLE_T INVALID_HANDLE = _INVALID_HANDLE;

public:
    basic_serial_device (void)
        : m_handle (INVALID_HANDLE)
    {
        basic_serial_device::log("Constructing new device at %p...", this);
        basic_serial_device::log("Constructed.");
    }

    virtual ~basic_serial_device (void) {
        basic_serial_device::log("Destructing device at %p...", this);
        this->close();
        basic_serial_device::log("Destructed.");
    }



    constexpr bool is_open (void) const {
        return m_handle != INVALID_HANDLE;
    }

    constexpr HANDLE_T get_handle (void) const {
        return m_handle;
    }



    bool open (const char *device) {
        basic_serial_device::log("Opening device at %p...", this);

        if (this->is_open()) {
            basic_serial_device::log("Device already open.");
        }
        else {
            m_handle = open_handle(device);
            basic_serial_device::log("%s device.", (this->is_open() ? "Successfully opened" : "Failed to open"));

            if (this->is_open()) {
                if (!this->configure())
                {
                    basic_serial_device::log("Failed to configure");
                    this->close();
                }
            }
        }

        return this->is_open();
    }

    bool close (void) {
        bool is_closed;
        basic_serial_device::log("Closing device at %p...", this);

        if (this->is_open()) {
            is_closed = close_handle(m_handle);
            if (is_closed) {
                m_handle = INVALID_HANDLE;
            }
            basic_serial_device::log("%s device.", (is_closed ? "Successfully closed" : "Failed to close"));
        }
        else {
            is_closed = true;
            basic_serial_device::log("Device is already closed.");
        }

        return is_closed;
    }

    // <0: error
    // >=0: # bytes read
    virtual ssize_t read (void *buffer, size_t size) = 0;

    // <0: error
    // >=0: # bytes written
    virtual ssize_t write (const void *buffer, size_t size) = 0;

    //ssize_t read_until (void *buffer, size_t buffer_size, void *terminator, size_t terminator_size) {}

    //ssize_t write_all (const void *buffer, size_t size) {}

    

    // <0: error
    // 0: timeout
    // >0: data available
    virtual int wait_for_data (size_t timeout_ms) = 0;



private:
    virtual HANDLE_T open_handle (const char *device) = 0;

    virtual bool close_handle (HANDLE_T) = 0;

    virtual bool configure (void) { return true; };



    template<typename... Args>
    static void log (const char (&fmt)[], Args&& ...args) {
        printf("[serial] ");
        printf(fmt, args...);
        printf("\n");
    }

    static void log (const char (&msg)[])
    {
        printf("[serial] %s\n", msg);
    }

    HANDLE_T m_handle;
};
