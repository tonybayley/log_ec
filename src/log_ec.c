/**
 * ****************************************************************************
 * @file   : log_ec.c
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

#include "log_ec.h"

/* Private macro definitions ------------------------------------------------*/

#ifndef LOG_USE_COLOR
#define LOG_USE_COLOR 0  /* Default: monochrom log printing */
#endif

/* Private type definitions -------------------------------------------------*/

#if LOG_USE_CALLBACKS
typedef struct {
    tLog_callbackFn cbFn;  //!< Callback function
    void* cbData;          //!< User data associated with callback function
    int cbLogLevel;        //!< Minimum logging level at which the callback is invoked
} tCallback;
#endif

typedef struct {
    void* lockData;                          //!< Application-specific data object required by lock function
    tLog_lockFn lockFn;                      //!< Lock function
    tLog_timestampFn timestampFn;            //!< Timestamp function
    int level;                               //!< Currently set logging level
    bool consoleLoggingDisabled;             //!< Flag to suppress printing of log messages to the console
#if LOG_USE_CALLBACKS
    tCallback callbacks[LOG_MAX_CALLBACKS];  //!< Array of logging callback functions
#endif
} tLogConfig;


/* Private variable definitions ---------------------------------------------*/

static tLogConfig logConfig = {
    .lockFn = NULL,
    .timestampFn = NULL,
    .level = LOG_TRACE,
    .consoleLoggingDisabled = false,
};

static const char* level_strings[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#if LOG_USE_COLOR
static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif


/* Private function declarations --------------------------------------------*/

static uint32_t getTimestamp( void );
static void log_print( tLog_event* ev );
static bool lock( void );
static bool unlock( void );


/* Private function definitions ---------------------------------------------*/

/**
 * @brief Get timestamp value.
 * 
 * @return Timestamp, as an unsigned integer value (uint32_t).
 */
static uint32_t getTimestamp( void )
{
    return ( NULL != logConfig.timestampFn ) ? logConfig.timestampFn() : 0U;
}

/**
 * @brief Write log event data to the console.
 * 
 * @param ev Log event data.
 */
static void log_print( tLog_event* ev )
{
#if LOG_USE_COLOR
    CONSOLE_PRINTF( "%10lu %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
            ev->time, level_colors[ev->level], level_strings[ev->level], ev->file, ev->line );  /* message prefix: colour */
#else
    CONSOLE_PRINTF( "%8lu %-5s %s:%d: ",
             ev->time, level_strings[ev->level], ev->file, ev->line );  /* message prefix: monochrome */
#endif
    CONSOLE_VPRINTF(ev->fmt, ev->ap);  /* message body */
}

static bool lock( void )
{
    bool lockAcquired = true;  /* if no lock function is set, lock acquisition always succeeds */
    if( NULL != logConfig.lockFn )
    {
        lockAcquired = logConfig.lockFn( true, logConfig.lockData );
    }
    return lockAcquired;
}

static bool unlock( void )
{
    bool lockReleased = true;  /* if no lock function is set, lock release always succeeds */
    if( NULL != logConfig.lockFn )
    {
        lockReleased = logConfig.lockFn( false, logConfig.lockData );
    }
    return lockReleased;
}


/* Public function definitions ----------------------------------------------*/

void log_set_lock_func( tLog_lockFn lockFn, void* lockData )
{
    logConfig.lockFn = lockFn;
    logConfig.lockData = lockData;
}

void log_set_timestamp_func( tLog_timestampFn timestampFn )
{
    logConfig.timestampFn = timestampFn;
}

void log_set_level( int level )
{
    logConfig.level = level;
}

void log_off( void )
{
    logConfig.consoleLoggingDisabled = true;
}

void log_on( void )
{
    logConfig.consoleLoggingDisabled = false;
}

#if LOG_USE_CALLBACKS
bool log_add_callback_func( tLog_callbackFn cbFn, void* cbData, int cbLogLevel )
{
    bool success = false;
    for( size_t i = 0; i < LOG_MAX_CALLBACKS; i++ )
    {
        if( ( !success ) && ( NULL == logConfig.callbacks[i].cbFn ) )
        {
            logConfig.callbacks[i] = (tCallback) { cbFn, cbData, cbLogLevel };
            success = true;
        }
    }
    return success;
}
#endif

void log_log( int level, const char* file, int line, const char* fmt, ... )
{
    tLog_event ev = {
        .level = level,
        .file  = file,
        .line  = line,
        .fmt   = fmt
    };
    ev.time = getTimestamp();

    bool lockAcquired = lock();

    if( lockAcquired )
    {
        /* write log messages to console */
        if( !logConfig.consoleLoggingDisabled && ( level >= logConfig.level ) )
        {
            va_start( ev.ap, fmt );
            log_print( &ev );
            va_end( ev.ap );
        }

#if LOG_USE_CALLBACKS
        /* invoke all registered logging callbacks */
        for( size_t i = 0; ( i < LOG_MAX_CALLBACKS ) && ( NULL != logConfig.callbacks[i].cbFn ); i++ )
        {
            tCallback* cb = &logConfig.callbacks[i];
            if ( level >= cb->cbLogLevel )
            {
                va_start( ev.ap, fmt );
                cb->cbFn( &ev, cb->cbData );
                va_end( ev.ap );
            }
        }
#endif

        unlock();
    }
}
