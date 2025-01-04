/**
 * ****************************************************************************
 * @file   : log_ec.h
 * @brief  : Logging library for embedded C
 * ****************************************************************************
 *
 * Copyright (c) 2025 Tony Bayley
 * Copyright (c) 2020 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef LOG_EC_H
#define LOG_EC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

/* Public macro definitions -------------------------------------------------*/

#define FILE_NAME ( strrchr( __FILE__, '/' ) ? strrchr( __FILE__, '/' ) + 1 : __FILE__ )

#define log_trace( ... ) log_log( LOG_TRACE, FILE_NAME, __LINE__, __VA_ARGS__ )
#define log_debug( ... ) log_log( LOG_DEBUG, FILE_NAME, __LINE__, __VA_ARGS__ )
#define log_info( ... )  log_log( LOG_INFO,  FILE_NAME, __LINE__, __VA_ARGS__ )
#define log_warn( ... )  log_log( LOG_WARN,  FILE_NAME, __LINE__, __VA_ARGS__ )
#define log_error( ... ) log_log( LOG_ERROR, FILE_NAME, __LINE__, __VA_ARGS__ )
#define log_fatal( ... ) log_log( LOG_FATAL, FILE_NAME, __LINE__, __VA_ARGS__ )

#ifndef LOG_MAX_CALLBACKS
#define LOG_MAX_CALLBACKS 0U  /* Default: logging callbacks are disabled */
#endif

/** Macro that evaluates 'true' if logging callbacks are enabled */
#define LOG_USE_CALLBACKS ( LOG_MAX_CALLBACKS > 0U )

#ifndef CONSOLE_PRINTF
#define CONSOLE_PRINTF( ... ) printf( __VA_ARGS__ )  /* Default: use printf() to write log message prefix to the console */
#endif

#ifndef CONSOLE_VPRINTF
#define CONSOLE_VPRINTF( FMT, ARG ) vprintf( FMT, ARG )  /* Default: use vprintf() to write log message body to the console */
#endif

/* Public type definitions --------------------------------------------------*/

/** Log event type */
typedef struct {
    uint32_t time;      //!< Timestamp value
    int level;          //!< Logging level of this log message
    const char *file;   //!< Filename
    int line;           //!< Line number
    const char* fmt;    //!< printf format string
    va_list ap;         //!< printf variadic arguments list
} tLog_event;

#if LOG_USE_CALLBACKS
/**
 * @brief Log callback function type.
 * 
 * The registered log callback function is invoked when a log message is
 * written at or above the currently set logging level.
 * 
 * @param ev Event struct that contains the log message and metadata.
 * @param cbData Pointer to application-specific callback data, if required, or NULL.
 */
typedef void (*tLog_callbackFn)( tLog_event* ev, void* cbData );
#endif

/**
 * @brief Log lock function type.
 * 
 * The optional log lock function implements thread-safe logging. When the lock 
 * function is called with lock=true, a mutex is acquired to prevent any other 
 * threads or RTOS tasks from writing log messages. When the lock function is 
 * called with lock=false, the mutex is released to allow other threads to write 
 * log messages.
 * 
 * @param lock true to acquire the mutex that enables printing of log messages, false to release the mutex.
 * @param lockData Pointer to application-specific data, if required, or NULL.
 * 
 * @return true if the mutex was successfully acquired or released, or false on failure.
 */
typedef bool (*tLog_lockFn)( bool lock, void* lockData );

/**
 * @brief Log timestamp function type.
 * 
 * The optional timestamp function returns the current time as an unsigned
 * integer value (uint32_t). The timestamp value units are application-specific,
 * but are normally milliseconds since boot-time.
 * 
 * @return timestamp value, as an unsigned integer.
 */
typedef uint32_t (*tLog_timestampFn)( void );

/**
 * @brief Log level enum.
 */
typedef enum {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} tLog_level;

/* Public function declarations ---------------------------------------------*/

/**
 * @brief Set the current logging level.
 * 
 * Only log messages at or above the currently set logging level are printed.
 * Log messages below the currently set logging level are suppressed.
 * 
 * @param level Currently set logging level.
 */
void log_set_level( int level );

/**
 * @brief Disable the printing of log messages to the console.
 */
void log_off( void );

/**
 * @brief Enable the printing of log messages to the console.
 */
void log_on( void );

#if LOG_USE_CALLBACKS
/**
 * @brief Register a logging callback function.
 *
 * The registered callback function will be invoked when a log message is written at a log level equal to or greater 
 * than  cbLogLevel. The parameters passed to the callback function are the log message and metadata, along with the 
 * callback's associated data object cbData.
 *
 * @param cbFn Logging callback function pointer.
 * @param cbData Logging callback data object pointer, if required, or NULL if not used.
 * @param cbLogLevel Lowest logging level at which the callback will be invoked.
 *
 * @return true on success, or false on failure (i.e. if the maximum number of callback functions has been exceeded).
 */
bool log_register_callback_func( tLog_callbackFn cbFn, void* cbData, int cbLogLevel );

/**
 * @brief Unegister a logging callback function.
 *
 * After unregistering a callback function, that function will no longer be invoked when log messages are written.
 *
 * @param cbFn Logging callback function pointer.
 */
void log_unregister_callback_func( tLog_callbackFn cbFn );
#endif

/**
 * @brief Register a function that generates timestamps.
 * 
 * @param timestampFn Timestamp function.
 * @return int 
 */
void log_set_timestamp_func( tLog_timestampFn timestampFn );

/**
 * @brief Register a logging lock function.
 *
 * The registered lock function acquires and releases a mutex to ensure that 
 * only a single thread or RTOS task can write log messages at any one time.
 * 
 * @param lockFn Lock function.
 * @param lockData Lock user data, if required, or NULL if not used.
 */
void log_set_lock_func( tLog_lockFn lockFn, void* lockData );

/**
 * @brief Main logging function.
 * 
 * @param level Logging level.
 * @param file Source file that is printing the log message.
 * @param line Source code line number that is printing the log message.
 * @param fmt printf format string.
 * @param ... printf variadic arguments.
 */
void log_log( int level, const char* file, int line, const char* fmt, ... );

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef LOG_EC_H */
