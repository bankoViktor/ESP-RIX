#define RIX_VERSION "0.3"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#warning RIX requires an ESP board. Disabling for non-supported board
#define RIX_DISABLE
#endif

/* Levels */
enum class RixLevels {
    Critical,
    Error,
    Warning,
    Information,
    Debug,
    Trace
};

///////////////////////////////////////////////////////////////////////
// The library is disabled so we null out a bunch of functions
///////////////////////////////////////////////////////////////////////
#ifdef RIX_DISABLE

// All the rix_# calls are nulled out
#define rix_critical(fmt, ...)
#define rix_error(fmt, ...)
#define rix_warn(fmt, ...)
#define rix_info(fmt, ...)
#define rix_debug(fmt, ...)
#define rix_trace(fmt, ...)

// Nulled out
#define rix_handle()
#define rix_log_level(x)
#define rix_color(x)
#define rix_delay(ms) delay(ms)
#define rix_init_wifi(x, y)
#define rix_tcp_port(x)

///////////////////////////////////////////////////////////////////////
// The library is enabled so business as usual
///////////////////////////////////////////////////////////////////////
#else

// Macros to wrap around debug print so we can capture the calling function name
#ifdef RIX_FORMAT_EX
#define rix_critical(fmt, ...) __debug_print2(RixLevels::Critical, fmt, ##__VA_ARGS__)
#define rix_error(fmt, ...)    __debug_print2(RixLevels::Error, fmt, ##__VA_ARGS__)
#define rix_warn(fmt, ...)     __debug_print2(RixLevels::Warning, fmt, ##__VA_ARGS__)
#define rix_info(fmt, ...)     __debug_print2(RixLevels::Information, fmt, ##__VA_ARGS__)
#define rix_debug(fmt, ...)    __debug_print2(RixLevels::Debug, fmt, ##__VA_ARGS__)
#define rix_trace(fmt, ...)    __debug_print2(RixLevels::Trace, fmt, ##__VA_ARGS__)
#else
#define rix_critical(fmt, ...) __debug_print(__func__, RixLevels::Critical, fmt, ##__VA_ARGS__)
#define rix_error(fmt, ...)    __debug_print(__func__, RixLevels::Error, fmt, ##__VA_ARGS__)
#define rix_warn(fmt, ...)     __debug_print(__func__, RixLevels::Warning, fmt, ##__VA_ARGS__)
#define rix_info(fmt, ...)     __debug_print(__func__, RixLevels::Information, fmt, ##__VA_ARGS__)
#define rix_debug(fmt, ...)    __debug_print(__func__, RixLevels::Debug, fmt, ##__VA_ARGS__)
#define rix_trace(fmt, ...)    __debug_print(__func__, RixLevels::Trace, fmt, ##__VA_ARGS__)
#endif

void   __debug_print(const char *func, RixLevels level, const char *format, ...);
void   __debug_print2(RixLevels level, const char *format, ...);
void   rix_handle();
void   rix_log_level(RixLevels level);
void   rix_delay(unsigned int ms);
void   rix_color(bool newState);
int    rix_init_wifi(const char *ssid, const char *password);
void   rix_tcp_port(int num);
String rix_ip2string(IPAddress ip);

#endif

// vim: tabstop=4 shiftwidth=4 noexpandtab autoindent softtabstop=4
