#ifndef _WIN32

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gps.h>
#include <math.h>


// https://kickstartembedded.com/2022/07/23/a-beginners-guide-to-using-gpsd-in-linux/
#define SERVER_NAME "localhost"
#define SERVER_PORT "2947"
struct gps_data_t g_gpsdata;
int ret;
int kickstartembedded_main (void) {
    // 1) Try GPS open
    ret = gps_open(SERVER_NAME,SERVER_PORT,&g_gpsdata);
    if(ret != 0) {
        printf("[GPS] Can't open... bye!\n");
        return -1;
    }

    // 2) Enable the JSON stream - we enable the watch as well
    (void)gps_stream(&g_gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);

    // 3) Wait for data from GPSD
    while (gps_waiting(&g_gpsdata, 5000000)) {
        sleep(2);
        if (-1 == gps_read(&g_gpsdata, NULL, 0)) {
            printf("Read error!! Exiting...\n");
            break;
        }
        else {
            if(g_gpsdata.fix.mode == MODE_2D || g_gpsdata.fix.mode == MODE_3D) {
                if(isfinite(g_gpsdata.fix.latitude) && isfinite(g_gpsdata.fix.longitude)) {
                    printf("[GPS DATA] Latitude, Longitude, Used satellites, Mode, Status = %lf, %lf, %d, %d, %d\n" , g_gpsdata.fix.latitude, g_gpsdata.fix.longitude,g_gpsdata.satellites_used,g_gpsdata.fix.mode,g_gpsdata.fix.status);
                }
                else {
                    printf(".");
                }
            }
            else {
                printf("Waiting for fix...\n");
            }
        }
    }

    // Close gracefully...
    (void)gps_stream(&g_gpsdata, WATCH_DISABLE, NULL);
    (void)gps_close(&g_gpsdata);
    return 0;
}



// https://gpsd.gitlab.io/gpsd/libgps.html
#define MODE_STR_NUM 4
static const char *mode_str[MODE_STR_NUM] = {
    "n/a",
    "None",
    "2D",
    "3D"
};
int gpsd_main (void)
{
    struct gps_data_t gps_data;

    if (int r = gps_open("localhost", "2947", &gps_data)) {
        if (const char *str = gps_errstr(r))
        {
            printf("%s\n", str);
        }
        else
        {
            printf("Open error.  Bye, bye\n");
        }
        return 1;
    }

    (void)gps_stream(&gps_data, WATCH_ENABLE | WATCH_JSON, NULL);

    while (gps_waiting(&gps_data, 5000000)) {
        if (-1 == gps_read(&gps_data, NULL, 0)) {
            printf("Read error.  Bye, bye\n");
            break;
        }

        if (MODE_SET != (MODE_SET & gps_data.set)) {
            // did not even get mode, nothing to see here
            printf("did not even get mode\n");
            continue;
        }

        if (0 > gps_data.fix.mode ||
            MODE_STR_NUM <= gps_data.fix.mode) {
            gps_data.fix.mode = 0;
        }

        printf("Fix mode: %s (%d) Time: ",
               mode_str[gps_data.fix.mode],
               gps_data.fix.mode);

        if (TIME_SET == (TIME_SET & gps_data.set)) {
            // not 32 bit safe
            printf("%ld.%09ld ", gps_data.fix.time.tv_sec,
                   gps_data.fix.time.tv_nsec);
        } else {
            puts("n/a ");
        }

        if (isfinite(gps_data.fix.latitude) &&
            isfinite(gps_data.fix.longitude)) {
            // Display data from the GPS receiver if valid.
            printf("Lat %.6f Lon %.6f\n",
                   gps_data.fix.latitude, gps_data.fix.longitude);
        } else {
            printf("Lat n/a Lon n/a\n");
        }
    }

    // When you are done...
    (void)gps_stream(&gps_data, WATCH_DISABLE, NULL);
    (void)gps_close(&gps_data);
    return 0;
}






int main (int argc, char *argv[]) {
    return kickstartembedded_main();
    return gpsd_main();
}

#else
#include <cstdio>
int main (void)
{
    printf("Win32 is not supported.\n");
    fflush(stdout);
    return 1;
}
#endif
