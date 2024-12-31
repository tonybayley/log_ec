# Logging library for embedded C

A simple logging library for embedded C.

This library is based on the _rxi_ logging library https://github.com/rxi/log.c
with modifications to make it suitable for resource-limited embedded systems.
Log messages are written to the console. Optionally, callback functions may be
registered to perform other logging actions such as writing logs to flash memory
or publishing logs via network connectivity.

![screenshot](https://cloud.githubusercontent.com/assets/3920290/23831970/a2415e96-0723-11e7-9886-f8f5d2de60fe.png)


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

Each macro prints the given format string to the console, prefixed with a 
timestamp, logging level, filename and line number:

```
21054 DEBUG src/main.c:11: Hello world
```


#### log_set_quiet(bool enable)
Logging to the console can be suppressed by passing `true` to the `log_set_quiet()` 
function. While enabled, the library does not write log messages to the console,
but continues to invoke logging callbacks if any are set.


#### log_set_level(int level)
The current logging level can be set by using the `log_set_level()` function.
`LOG_TRACE` is the lowest (most detailed) logging level and `LOG_FATAL` is the
highest (least detailed) logging level. Only logging macros at or above the 
current logging level write to the console. By default the current logging level 
is set to `LOG_TRACE` at boot time, such that nothing is ignored.


#### log_add_callback(log_LogFn fn, void *udata, int level)
One or more callback functions which are called with the log data can be
registered by calling the `log_add_callback()` function. When logging macros at
or above the current logging level are called, the registered callback functions 
are invoked with a `log_Event` structure containing the timestamp, logging 
level, filename, line number, format string, printf va\_list and pointer to an
application-specific data object (e.g. flash filesystem path).


#### log_set_lock(log_LockFn fn, void *udata)
If the log will be written to from multiple threads or RTOS tasks, a lock
function can be set. The function is passed the boolean `true` if the lock 
should be acquired or `false` if the lock should be released, along with a
pointer to an application-specific data object (e.g. a mutex handle).


#### const char* log_level_string(int level)
Returns the name of the given log level as a string.


#### LOG_USE_COLOR
If the library is compiled with `-DLOG_USE_COLOR` ANSI color escape codes will
be used when printing.


## License
This library is free software; you can redistribute it and/or modify it under
the terms of the MIT license. See [LICENSE](LICENSE) for details.
