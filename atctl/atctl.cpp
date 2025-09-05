#include "../serial/serial.h"
#include "../source_exception/source_exception.h"
#include "../common.h"
#include "string_manip.h"

#include <iostream>
#include <cstdio>
#include <fcntl.h>
#include <cerrno>
#include <string>
#include <cstring>
#include <filesystem>
#include <vector>
#ifndef _WIN32
    #include <csignal>
    #include <csetjmp>
    #include <unistd.h>
    #include <sys/select.h>
#endif



#define RED             "\x1b[31m"
#define GREEN           "\x1b[32m"
#define YELLOW          "\x1b[33m"
#define BRIGHT_YELLOW   "\x1b[93m"
#define DEFAULT         "\x1b[39m"




static bool interactive = false;
static bool raw = false;


#ifndef _WIN32
template <typename _CharT, typename _Traits, typename _Alloc>
bool interruptable_getline (std::basic_istream<_CharT, _Traits> &__is, std::basic_string<_CharT, _Traits, _Alloc> &__str)
{
    static thread_local std::jmp_buf ctx_before_getline;

    bool was_interrupted;
    auto prev_signal_handler = signal(SIGINT,
        [](int signum) {
            DBG("Calling std::longjmp()");
            std::longjmp(ctx_before_getline, 1);
        });



    if (setjmp(ctx_before_getline))
    {
        signal(SIGINT, prev_signal_handler); // restore previous handler
        was_interrupted = true;
    }
    else
    {
        DBG("Calling std::getline()", 0);
        std::getline(__is, __str);
        was_interrupted = false;
    }



    return !was_interrupted;
}
#endif



static void _send_at_command (serial_device &conn, const std::string &command)
{
    // Write full command.
    const std::string message = "AT" + command + "\r";
    const ssize_t n_written = conn.write(message.c_str(), message.size());

    if (-1 == n_written)
    {
        throw source_exception("Failed to write to device");
    }
    else if (n_written < message.size())
    {
        throw source_exception("Failed to write full command");
    }



    // Read response.
    std::string response;
    bool terminator_found = false;
    char buffer [128];
    ssize_t n_read;
    bool data_available;

    while (!terminator_found)
    {
        // Async wait (if possible) for data to arrive.
        const int rv = conn.wait_for_data(30000);
        if (rv < 0)
        {
            throw source_exception("Failed to wait for data");
        }
        else if (rv == 0)
        {
            printf("Timed out.\n");
            break;
        }
        else
        {
            // Read all available data.
            data_available = true;
            while (!terminator_found && data_available)
            {
                n_read = conn.read(buffer, sizeof(buffer));
                if (n_read < 0)
                {
                    throw source_exception("Failed to read from device");
                }
                else
                {
                    if (n_read > 0)
                    {
                        response.append(buffer, n_read);

                        for (const auto &term : {"OK\r\n", "ERROR\r\n"})
                        {
                            if (response.ends_with(term))
                            {
                                terminator_found = true;
                                break;
                            }
                        }
                    }

                    if (n_read < sizeof(buffer))
                    {
                        data_available = false;
                    }
                }
            }
        }
    }



    // Print response.
    if (raw)
    {
        printf("%s\n", response.c_str());
    }
    else
    {
        // omit first line b/c it echoes the input
        for (auto &line : split(response, 1))
        {
            // remove leading/trailing whitespace
            strip(line);

            // omit empty lines
            if (!line.empty())
            {
                if (line == "OK")
                {
                    line.insert(0, GREEN).append(DEFAULT);
                }
                else if (line == "ERROR")
                {
                    line.insert(0, RED).append(DEFAULT);
                }

                printf("   %s\n", line.c_str());
            }
        }
    }
}

static void send_at_command (serial_device &device, const std::string &command)
{
    _send_at_command(device, command);
}

static void send_at_command_interactive (serial_device &device, std::string &first_command)
{
#ifdef _WIN32
    printf(BRIGHT_YELLOW " >>> Interactive mode. Enter q to quit. <<<" DEFAULT "\n");
#else
    printf(BRIGHT_YELLOW " >>> Interactive mode. Ctrl+c or q to quit. <<<" DEFAULT "\n");
#endif


    strip(first_command);
    if (!first_command.empty())
    {
        printf("\n" YELLOW " > AT" BRIGHT_YELLOW "%s" DEFAULT "\n", first_command.c_str());
        _send_at_command(device, first_command);
    }


    std::string command;
    while (1)
    {
        printf("\n" YELLOW " > AT" BRIGHT_YELLOW);

#ifdef _WIN32
        std::getline(std::cin, command);
#else
        if (!interruptable_getline(std::cin, command))
        {
            printf(DEFAULT "\nUser requested stop.\n");
            break;
        }
#endif

        printf(DEFAULT);

        if (command == "q" || command == "Q")
        {
            break;
        }

        _send_at_command(device, command);
    }
}



static std::false_type usage (const char *detail = nullptr)
{
    static const char USAGE_MESSAGE [] =
        "Usage: atctl [options] <device> [command]\n"
        "  device       A serial device with which to send AT-Commands.\n"
        "  command      An AT-Command to issue (without AT prefix). If\n"
        "               omitted, interactive mode will be used.\n"
        "\n"
        "  options:\n"
        "    -r         Print the raw unfiltered response.\n"
        "    -i         Interactive mode.\n"
        "    -h, --help\n"
        "\n"
        "  Examples:\n"
        "    atctl /dev/ttyUSB0 GSTATUS?\n"
        "    atctl -i /dev/ttyUSB0\n"
        ;

    if (detail)
    {
        fprintf(stderr, RED "%s" DEFAULT "\n", detail);
    }

    fprintf(stderr, "%s\n", USAGE_MESSAGE);
    return std::false_type();
}

static bool parse (int argc, char *argv[], const char *&device_dest, std::string &command_dest)
{
    constexpr size_t N_REQ = 1;
    char *req_positional [N_REQ];
    size_t req_count = 0;

    constexpr size_t N_OP_MAX = 1;
    char *op_positional [N_OP_MAX];
    size_t op_count = 0;



    // check if requesting help
    if (argc < 2)
    {
        return usage(); // no args
    }

    for (const std::string &k : {"-h", "--help"})
    {
        if (k == argv[1])
        {
            return usage();
        }
    }



    // parse
    for (int i = 1; i < argc; i++)
    {
        char *arg = argv[i];


        // optional flags
        if (arg[0] == '-')
        {
            if (0 == strncmp("-r", arg, 3))
            {
                raw = true;
            }
            else if (0 == strncmp("-i", arg, 3))
            {
                interactive = true;
            }
            else
            {
                const auto str = std::string("Unrecognized option: ").append(arg);
                return usage(str.c_str());
            }
        }

        // required positional args
        else if (req_count < N_REQ)
        {
            req_positional[req_count++] = arg;
        }

        // optional positional args
        else if (op_count < N_OP_MAX)
        {
            op_positional[op_count++] = arg;
        }

        // unknown arg (all positional args have been supplied)
        else
        {
            const auto str = std::string("Unrecognized argument: ").append(arg);
            return usage(str.c_str());
        }
    }

    // Make sure all required args were supplied.
    if (req_count < N_REQ)
    {
        static const char *keys[] = {"device"};

        const auto str = std::string("Missing required argument: ").append(keys[req_count]);
        return usage(str.c_str());
    }
    else
    {
        device_dest = req_positional[0];
    }

    // Handle any extra args.
    if (op_count > 0)
    {
        command_dest = op_positional[0];
    }
    else
    {
        interactive = true;
    }


    return true;
}

int main (int argc, char *argv[])
{
    const char *device_path;
    std::string command;
    int rc = EXIT_FAILURE;



    // Parse command line args.
    if (parse(argc, argv, device_path, command))
    {
        try
        {
            serial_device at_device;
            if (at_device.open(device_path))
            {
                if (interactive)
                {
                    send_at_command_interactive(at_device, command);
                }
                else
                {
                    send_at_command(at_device, command);
                }

                at_device.close();
                rc = EXIT_SUCCESS;
            }
        }
        catch (const source_exception &e)
        {
            fprintf(stderr, "%s\n", e.what());
        }
        catch (const std::exception &e)
        {
            fprintf(stderr, "Unexpected exception: %s\n", e.what());
        }
    }



    printf(DEFAULT);
    fflush(stderr);
    fflush(stdout);
    return rc;
}
