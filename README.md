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


## Logging API functions

### log_set_level( int level )

The current logging level can be set by using the `log_set_level()` function.
`LOG_TRACE` is the lowest (most detailed) logging level and `LOG_FATAL` is the
highest (least detailed) logging level. Only log messages at or above the 
logging level set by this function are written to the console. By default the
logging level is initialised to `LOG_TRACE` at boot time, so that log messages
at all levels are displayed.


### log_set_quiet( bool quiet )

Logging to the console can be suppressed by calling `log_set_quiet()` with
parameter `quiet=true`, and can be enabled again by calling `log_set_quiet()` 
with parameter `quiet=false`. While _quiet_ mode is active, the library does not 
write log messages to the console but continues to invoke logging callbacks if 
any are set.


### log_set_timestamp_func( tLog_timestampFn timestampFn )

A function that generates timestamps may be set by calling `log_set_timestamp_func()`,
with the parameter `timestampFn` set to the timestamp function pointer. The 
timestamp function returns unsigned integer values (uint32_t). The units are
user-defined, but are typically set to elapsed milliseconds since boot-time. For
example, the following code-snippet shows how to configure timestamps in units
of milliseconds on a FreeRTOS based system:

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
    /* initialise system clocks and hardware */
    ...
    log_set_timestamp_func( getTimestamp );  // called once only at boot-time, to register the timestamp function
```


### log_add_callback_func( tLog_callbackFn cbFn, void* cbData, int cbLogLevel )

If the preprocessor macro `LOG_MAX_CALLBACKS` is set to a non-zero positive 
integer, then up to `LOG_MAX_CALLBACKS` logging callback functions can be 
registered by calling the `log_add_callback()` function. The function parameters 
are:

- **cbFn**: Logging callback function pointer.
- **cbData**: Pointer to user data, if required, or NULL if the callback does not require user data.
- **cbLogLevel**: Lowest logging level at which the callback will be invoked.

When a logging callback function is invoked, the `ev` parameter points to a 
struct of type `tLog_event` that contains the log message and all associated 
metadata, and the `cbData` parameter is a pointer to the user data object that 
was specified when the callback was registered.

Logging callback functions enable target-specific logging features to be
implemented, such as writing ERROR logs to a flash filesystem, or publishing
log messages via an MQTT broker.


### log_set_lock_func( tLog_lockFn lockFn, void* lockData )

If the log will be written to from multiple threads or RTOS tasks, the user can
supply a lock function to prevent interleaving of messages from different 
threads in the console output. To do this, the function `log_set_lock_func()` is
called with parameter `lockFn` as the lock function pointer and parameter
`lockData` as a pointer to a user data object (e.g. a mutex handle) if required
by the lock function, or NULL if user data is not required.


The logging functions call the user-supplied lock function with `lock=true` to 
acquire a mutex before writing to the log, and call it again with `lock=false`
to release the mutex when log writing is complete.

The lock function is more complex if the log is written from ISR context 
(Interrupt Service Routines) as well as from RTOS tasks because ISRs must not
block. When the lock function is called from RTOS task context, the task blocks 
until the mutex can be acquired if the mutex is currently held by another task. 
Log writing may be delayed, but is guaranteed to occur. However, when the lock 
function is called from ISR context, it should return immediately with return 
value `false` if it cannot acquire the mutex. Therefore log messages from ISR 
context may be discarded if a task is writing to the log at the same time and 
has ownership of the mutex.

The following code snippet shows how to configure a lock function on a FreeRTOS
based system that uses an ARM Cortex-M processor. The FreeRTOS ports for many
ARM Cortex-M processors include a macro `xPortIsInsideInterrupt()` that can be
used to determine whether the system is in ISR or Task context, so that the 
appropriate context-specific lock behaviour can be implemented.

```c
#include "log_ec.h"
#include "FreeRTOS.h"

static SemaphoreHandle_t logMutex;

/**
 * @brief Log lock function.
 * 
 * @param lock true to acquire the mutex that enables printing of log messages, false to release the mutex.
 * @param lockData Pointer to application-specific data, if required, or NULL.
 * 
 * @return true if the mutex was successfully acquired or released, or false on failure.
 */
static bool logLockFunction( bool lock, void* lockData )
{
    (void) lockData;  // not used
    bool success = false;
    if( logMutex != NULL )
    {
        if( pdTRUE == xPortIsInsideInterrupt() )
        {
            /* FreeRTOS is in ISR context */
            if( lock )
            {
                success = ( pdTRUE == xSemaphoreTakeFromISR( logMutex, NULL ) );
            }
            else
            {
                success = ( pdTRUE == xSemaphoreGiveFromISR( logMutex, NULL ) );
            }           
        }
        else
        {
            /* FreeRTOS is in TASK context*/
            if( lock )
            {
                success = ( pdTRUE == xSemaphoreTake( logMutex, portMAX_DELAY ) );
            }
            else
            {
                success = ( pdTRUE == xSemaphoreGive( logMutex ) );
            }
        }
    }
    return success;
}

int main( void )
{
    /* initialise system clocks and hardware */
    ...
    logMutex = xSemaphoreCreateMutex();
    if( NULL == logMutex )
    {
        log_fatal( "Failed to create logMutex\n" );
        Error_Handler();
    }
    log_set_lock_func( logLockFunction, NULL );
```


## Compile time options

### Color

If the library is compiled with preprocessor macro `LOG_USE_COLOR` set to 1, 
ANSI color escape codes will be used when printing. If the macro is set to 0, or
is undefined, then monochrome printing will be used.

If you are building with CMake, then setting the `LOG_USE_COLOR` CMake cache 
variable to "1" or "0" causes that value to be assigned to the `LOG_USE_COLOR` 
preprocessor macro.

### Logging callback functions

If the library is compiled with preprocessor macro `LOG_MAX_CALLBACKS` set to a
non-zero positive integer, then user-defined logging callback functions are 
supported and any number of callback functions between 1 and `LOG_MAX_CALLBACKS` 
may be registered by calling `log_add_callback_func()`. If the `LOG_MAX_CALLBACKS`
macro is set to 0, or is undefined, then user-defined logging callback functions 
are not supported.

If you are building with CMake, then setting the `LOG_MAX_CALLBACKS` CMake cache 
variable to a positive integer string value causes the same value to be assigned
to the `LOG_MAX_CALLBACKS` preprocessor macro.


## Building the log_ec library with CMake

This git repository contains a [**CMakeLists.txt**](./CMakeLists.txt) file that
builds _log\_ec_ as a static library. To include the _log\_ec_ logging library 
in another application, add the following code snippet to that application's
top-level _CMakeLists.txt_ file:

```cmake
FetchContent_Declare(log_ec
    GIT_REPOSITORY  git@bitbucket.org:tbayley/log_ec.git
    GIT_TAG         main
    SOURCE_DIR      ${CMAKE_CURRENT_LIST_DIR}/src/log_ec
)

FetchContent_MakeAvailable( log_ec )

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    log_ec
)
```


## License

This library is free software; you can redistribute it and/or modify it under
the terms of the MIT license. See [LICENSE](LICENSE) for details.
