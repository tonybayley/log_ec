/**
 * ****************************************************************************
 * @file   : console_printf.h
 * @brief  : printf() and vprintf() override macro definitions for tests
 * ****************************************************************************
 *
 * Copyright (c) 2025 Tony Bayley
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

#ifndef CONSOLE_PRINTF_H
#define CONSOLE_PRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

/* Public macro definitions -------------------------------------------------*/

/**
 * @brief Macro to override printf() statndard library function.
 * 
 * @param format Print format string.
 * @param ...    Variable argument array.
 * @return Number of characters written. 
 */
#define CONSOLE_PRINTF( FMT, ... ) testPrintf( FMT, __VA_ARGS__ )

/**
 * @brief Macro to override vprintf() statndard library function.
 * 
 * @param format Print format string.
 * @param arg Argument list.
 * @return Number of characters written. 
 */
#define CONSOLE_VPRINTF( FMT, ARG ) testVprintf( FMT, ARG )  /* override vprintf() with function that writes to test buffer */

/* Public function declarations **********************************************/

/**
 * @brief Function to override printf() that writes log messages to a test buffer.
 * 
 * @param format Print format string.
 * @param ...    Variable argument array.
 * @return Number of characters written. 
 */
int testPrintf( const char* format, ...);

/**
 * @brief Function to override vprintf() that writes log messages to a test buffer.
 * 
 * @param format Print format string.
 * @param arg Argument list.
 * @return Number of characters written. 
 */
int testVprintf( const char* format, va_list arg);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef CONSOLE_PRINTF_H */
