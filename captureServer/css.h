#ifndef __CSS_H__
#define __CSS_H__

#include "uv/uv.h"
#include <fcntl.h>
#include <stdio.h>

#ifdef _WIN32
# include <direct.h>
# include <io.h>
# ifndef S_IRUSR
#  define S_IRUSR _S_IREAD
# endif
# ifndef S_IWUSR
#  define S_IWUSR _S_IWRITE
# endif
# define unlink _unlink
# define rmdir _rmdir
# define stat _stati64
# define open _open
# define write _write
# define lseek _lseek
# define close _close
# define snprintf _snprintf
# define vsnprintf _vsnprintf
#else
# include <unistd.h> /* unlink, rmdir, etc. */
# define Sleep(T) sleep((T)/1000)
# ifndef O_BINARY
#  define O_BINARY 0
# endif
#endif

#ifndef MAX_PATH
# define MAX_PATH 256
#endif

#define NO_IMPL_INT(fun) return printf(#fun " no implement\n")
#define NO_IMPL(fun) printf(#fun " no implement\n")


#define CSS_DEFAULT_PORT_STR "5566"
#define CSS_DEFAULT_CONFIG_NAME "css"

#define CL_INFO printf
#define CL_ERROR printf

#ifdef __cplusplus
extern "C"{
#endif 

    /*
     * stream media server info DTO
     */
    typedef struct css_sms_info_s
    {
        int64_t smsEquipId;
        struct sockaddr_in smsAddr;
    } css_sms_info_t;

    /*
     * the server's config
     */
    typedef struct css_config_s
    {
        uv_loop_t *loop;
        uint16_t listen_port;
    } css_config_t;

    /*
     * function:
     */
    int css_server_init(const char* conf_path, int index);
    int css_server_start(void);
    int css_server_stop(void);
    void css_server_clean(void);

#ifdef __cplusplus
}
#endif 

#endif /* __CSS_H__ */
