#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdarg.h>

#include <unistd.h>

#include <log.h>

static const uint32_t MAX_LOG_LENGTH = 0x1000;
static const int STDERR_FD = 2;
static llt_L s_L;

void llt_init()
{
    s_L.f = NULL;
    s_L.fd = -1;
    char * lvl_msk = getenv("LLT_LOG_LEVEL");

    if(lvl_msk) s_L.lvl_msk = atoi(lvl_msk); 
    else s_L.lvl_msk = 0;
}

void llt_log_set_file(FILE *file)
{
    s_L.f = file;
}

void llt_log_set_fd(int fd)
{
    s_L.fd = fd;
}

// size_t __llt_log(int lvl_msk, char **buf, char const *format, va_list args)
// {
//     int bufferSize = MAX_LOG_LENGTH;
//     // char* buf = NULL;

//     for (;;)
//     {
//         *buf = (char*)malloc(bufferSize);

//         if (*buf == NULL)
//         {
//             return 0;    // not enough memory
//         }

//         int ret = vsnprintf(*buf, bufferSize - 3, format, args);

//         if (ret < 0)
//         {
//             bufferSize *= 2;

//             free(*buf);
//         }
//         else
//         {
//             break;
//         }

//     }

//     strcat(*buf, "\n");

//     // Linux, Mac, iOS, etc
//     // fprintf(stderr, "%s", buf);
//     // fflush(stderr);

//     // free(buf);

//     return strlen(*buf);
// }

void llt_log(int lvl_msk, const char * format, ...)
{
    if( !(s_L.lvl_msk & lvl_msk) ) return;

    va_list args;
    char *buf = NULL;
    va_start(args, format);
    int bufferSize = MAX_LOG_LENGTH;

    for (;;)
    {
        buf = (char*)malloc(bufferSize);

        if (buf == NULL)
        {
            return 0;    // not enough memory
        }
        int ret = vsnprintf(buf, bufferSize - 3, format, args);

        if (ret < 0)
        {
            bufferSize *= 2;

            free(buf);
        }
        else
        {
            break;
        }
    }
    strcat(buf, "\n");
    va_end(args);

    size = strlen(buf);

    if(size)
    {
        if( s_L.lvl_msk & 0xF ) write(STDERR_FD, buf, size);
        if( s_L.lvl_msk & 0xF ) write(s_L.fd, buf, size);
    }

    if(size) free(buf);
}