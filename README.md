# Logging library for embedded C

A simple logging library for embedded C.

This library is based on the _rxi_ logging library https://github.com/rxi/log.c
with modifications to make it suitable for resource-limited embedded systems.
Log messages are written to the console. Optionally, callback functions may be
registered to perform other logging actions such as writing logs to flash memory
or publishing logs via network connectivity.

The embedded target must implement the printf() function. On embedded systems, 
the printf() function is typically configured to print to a UART, USB virtual 
COM port or J-Link RTT channel that can be viewed on an attached PC.

```bash
   34425 TRACE usbd_cdc.c:698: USB CDC hcdc->TxState = 0 (IDLE)
   34796 TRACE usbd_cdc.c:698: USB CDC hcdc->TxState = 0 (IDLE)
   34996 TRACE usbd_cdc.c:698: USB CDC hcdc->TxState = 0 (IDLE)
   36587 TRACE usbd_cdc.c:698: USB CDC hcdc->TxState = 0 (IDLE)
   37054 DEBUG led.c:57: cdcTransmitResult = 0
   38054 DEBUG led.c:57: cdcTransmitResult = 0
   39054 DEBUG led.c:57: cdcTransmitResult = 0
   40054 DEBUG led.c:57: cdcTransmitResult = 0
```

## Usage
**[log_ec.c](src/log_ec.c)** and **[log_ec.h](src/log_ec.h)** should be added to
an existing project and compiled along with it. The library provides six
function-like macros for logging:

```c
log_trace(const char* fmt, ...);
log_debug(const char* fmt, ...);
log_info(const char* *fmt, ...);
log_warn(const char* fmt, ...);
log_error(const char* fmt, ...);
log_fatal(const char* fmt, ...);
```

Each macro takes a printf format string followed by additional arguments:

```c
log_debug( "Hello %s%c", "worl", 'd' );
```

The macros print the given format string to the console, prefixed with a 
timestamp, logging level, filename and line number:

```
   21054 DEBUG src/main.c:11: Hello world
```

### Timestamps
Timestamps may be included in log messages by calling `log_set_timestamp_func()`
to register a function that returns timestamp values. For example, the following
code snippet shows how to implement, register and use a function to provide 
timestamps in units of milliseconds for a FreeRTOS based system:

```c
#include "log_ec.h"
#include "FreeRTOS.h"

static uint32_t getTimestamp( void )
{
    TickType_t time = pdTICKS_TO_MS( xTaskGetTickCount() );
    return (uint32_t)time;
}

int main(void)
{
    log_set_timestamp_func( getTimestamp );  // called once only, to register the timestamp function
    ...
    log_info( "Entering %s function\n", __func__ );
    ...
}
```

### Compile time options

#### Color
If the library is compiled with preprocessor macro `LOG_USE_COLOR` set to 1, 
ANSI color escape codes will be used when printing. If the macro is set to 0, or
is undefined, then monochrome printing will be used.

#### Logging callback functions
If the library is compiled with preprocessor macro `LOG_MAX_CALLBACKS` set to a
non-zero positive integer, then user-defined logging callback functions are 
supported and any number of callback functions between 1 and `LOG_MAX_CALLBACKS` 
may be registered by calling `log_add_callback_func()`. If the `LOG_MAX_CALLBACKS`
macro is set to 0, or is undefined, then user-defined logging callback functions 
are not supported.


### Logging functions

#### log_set_quiet( bool enable )
Logging to the console can be suppressed by passing `true` to the `log_set_quiet()` 
function. While enabled, the library does not write log messages to the console,
but continues to invoke logging callbacks if any are set.


#### log_set_level( int level )
The current logging level can be set by using the `log_set_level()` function.
`LOG_TRACE` is the lowest (most detailed) logging level and `LOG_FATAL` is the
highest (least detailed) logging level. Only logging macros at or above the 
currently set logging level write to the console. By default the current logging 
level is set to `LOG_TRACE` at boot time, such that nothing is ignored.


#### log_set_timestamp_func( tLog_timestampFn fn )
A function that generates timestamps may be set by calling `log_set_timestamp_func()`.
The timestamp function returns unsigned integer values (uint32_t). The units are
user-defined, but are often set to elapsed milliseconds since boot-time.


#### log_add_callback_func( tLog_callbackFn cbFn, void* cbData, int cbLogLevel )
One or more logging callback functions can be registered, along with an  
associated data object `cbData` and logging level `cbLogLevel`, by calling the 
`log_add_callback()` function. When log messages are written with logging level 
at or above the callback function's associated `cbLogLevel`, the callback 
function is invoked. The callback is passed a `tLog_event` parameter containing 
the timestamp, logging level, filename, line number, format string, printf 
va\_list and a pointer to the callback's associated `cbData` object.


#### log_set_lock_func( tLog_lockFn lockFn, void* lockData )
If the log will be written to from multiple threads or RTOS tasks, a lock
function can be set. The function is passed the boolean `true` if the lock 
should be acquired or `false` if the lock should be released, along with a
pointer to an application-specific data object (e.g. a mutex handle).


## License
This library is free software; you can redistribute it and/or modify it under
the terms of the MIT license. See [LICENSE](LICENSE) for details.
