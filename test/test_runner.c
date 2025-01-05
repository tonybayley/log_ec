/**
 * ****************************************************************************
 * @file   : test_log_ec.c
 * @brief  : Unit tests for log_ec logging library
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

#include <stdlib.h>
#include <errno.h>
#include "log_ec.h"


/* Private macro definitions ------------------------------------------------*/

#define EXPECTED_NUMBER_OF_ARGS 2    /* Expected number of command line arguments, including the executable command */


/* Private type definitions -------------------------------------------------*/

/**
 * @brief Test function pointer type
 * 
 * @return 0 if the test passes
 *         1 if the test fails
 */
typedef int (*tTestFunction)( void );


/* Private function declarations --------------------------------------------*/

static int test_log_trace_messageFormat( void );
static int test_log_debug_messageFormat( void );


/* Private variable definitions ---------------------------------------------*/

/**
 * @brief List of unit test functions
 */
tTestFunction pTest[] = {
    test_log_trace_messageFormat,
    test_log_debug_messageFormat
};


/* Public function definitions ----------------------------------------------*/

/**
 * @brief Test runner main function.
 * 
 * The test runner is invoked on the command line, and is passed a single 
 * integer that specifies the test number to run. For example:
 * 
 *     ./TestRunner 5
 * 
 * @param argc Number of command line arguments
 * @param argv Command line argument array
 * @return 0 if test passes
 *         1 if test fails
 */
int main(int argc, char* argv[])
{
    int result = 1;  // test failed
    if( EXPECTED_NUMBER_OF_ARGS == argc )
    {
        errno = 0;
        long testNumber = strtol( argv[1U], NULL, 10 );
        if( ( errno == 0 ) && ( testNumber < sizeof(pTest) / sizeof(pTest[0U]) ) )
        {
            result = pTest[testNumber]();
        }
    }
    return result;
}


/* Private function definitions ---------------------------------------------*/

/**
 * @brief Verify the format of log messages printed by log_trace() macro.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_trace_messageFormat( void )
{
    return 0;  // pass
}

/**
 * @brief Verify the format of log messages printed by log_trace() macro.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_debug_messageFormat( void )
{
    return 0;  // pass
}
