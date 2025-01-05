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

#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include "log_ec.h"


/* Private macro definitions ------------------------------------------------*/

#define EXPECTED_NUMBER_OF_ARGS 2           /* Expected number of command line arguments, including the executable command */
#define DEFAULT_EXPECTED_TIMESTAMP 12345U   /* Default timestamp value that is expected to be included in log message */
#define TEST_BUFFER_SIZE 80U                /* Size in bytes of the test buffer to which log messages are written*/
#define NEXT_LINE ( __LINE__ + 1 )          /* Line number of the next line */

/**
 * @brief Compare a log message with the expected message
 * 
 * @return 0 if the messages are identical, a non-zero value if the messages are different.
 */
#define TEST_ASSERT_EQUAL( EXPECTED_MESSAGE, MESSAGE ) \
        strcmp( EXPECTED_MESSAGE, MESSAGE )


/* Private type definitions -------------------------------------------------*/

/**
 * @brief Test function pointer type.
 * 
 * @return 0 if the test passes
 *         1 if the test fails
 */
typedef int (*tTestFunction)( void );

/**
 * @brief Test item type.
 */
typedef struct {
    const char* testName;
    tTestFunction testFunction;
} tTestItem;

/* Private function declarations --------------------------------------------*/

static void setExpectedTimestamp( uint32_t expectedTimestamp );
static uint32_t getTimestamp( void );
static void clearLogMessage( void );
static int test_log_trace_messageFormat( void );
static int test_log_debug_messageFormat( void );


/* Private variable definitions ---------------------------------------------*/

/** @brief List of unit test functions */
tTestItem testList[] = {
    { "log_trace message format", test_log_trace_messageFormat },
    { "log_debug message format", test_log_debug_messageFormat },
};

/** Number of test cases */
const size_t testListLength = sizeof( testList ) / sizeof( testList[0U] );

/** Expected timestamp value */
uint32_t timestamp = 0U;

/** Test buffer to which log messages are written */
char logMessage[TEST_BUFFER_SIZE] = { '\0'};

/** Test buffer write index */
size_t logMessageWriteIndex = 0U;

/* Public function definitions ----------------------------------------------*/

/**
 * @brief Test runner main function.
 * 
 * The test runner is invoked on the command line, and is passed a single 
 * integer that specifies the test number to run. For example:
 * 
 *     ./TestRunner test_log_debug_messageFormat
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
        bool testComplete = false;
        const char* testName = argv[1U];
        for( size_t i = 0U; ( i < testListLength ) && ( !testComplete ); i++ )
        {
            if( 0 == strcmp( testName, testList[i].testName ) )
            {
                // test setup
                log_set_timestamp_func( getTimestamp );
                setExpectedTimestamp( DEFAULT_EXPECTED_TIMESTAMP );
                clearLogMessage();

                // run the test with the specified funcion name
                result = testList[i].testFunction();
                testComplete = true;
            }
        }
    }
    return result;
}

int testPrintf( const char* format, ...)
{
    va_list arg;
    va_start( arg, format );
    size_t freeSpace = TEST_BUFFER_SIZE - logMessageWriteIndex;
    int bytesWritten = vsnprintf( &logMessage[logMessageWriteIndex], freeSpace , format, arg );
    logMessageWriteIndex += bytesWritten;
    va_end( arg );
    return bytesWritten;
}

int testVprintf( const char* format, va_list arg)
{
    size_t freeSpace = TEST_BUFFER_SIZE - logMessageWriteIndex;
    int bytesWritten = vsnprintf( &logMessage[logMessageWriteIndex], freeSpace, format, arg );
    logMessageWriteIndex += bytesWritten;
    return bytesWritten;
}


/* Private function definitions ---------------------------------------------*/

/**
 * @brief Set the timestamp value that will be included in the next log message.
 * 
 * @param expectedTimestamp Expected timestamp value.
 */
static void setExpectedTimestamp( uint32_t expectedTimestamp )
{
    timestamp = expectedTimestamp;
}

/**
 * @brief Custom timestamp generator function that returns a known timestamp value when running tests.
 * 
 * @return timestamp value
 */
static uint32_t getTimestamp( void )
{
    return timestamp;
}

/**
 * @brief Clear the log message buffer.
 */
static void clearLogMessage( void )
{
    memset( logMessage, 0, TEST_BUFFER_SIZE );
    logMessageWriteIndex = 0U;
}

/* Test cases ---------------------------------------------------------------*/

/**
 * @brief Verify the format of log messages printed by log_trace() macro.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_trace_messageFormat( void )
{
    int testValue = 48;
    char expectedLogMessage[80] = { '\0'};
    sprintf( expectedLogMessage, "   12345 TRACE test_runner.c:%u: testValue is 48\n", NEXT_LINE );
    log_trace( "testValue is %d\n", testValue );
    int result = TEST_ASSERT_EQUAL( expectedLogMessage, logMessage );
    return result;
}

/**
 * @brief Verify the format of log messages printed by log_trace() macro.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_debug_messageFormat( void )
{
    unsigned int testValue = 0xFACE;
    char expectedLogMessage[80] = { '\0'};
    sprintf( expectedLogMessage, "   12345 DEBUG test_runner.c:%u: testValue is 0xFACE\n", NEXT_LINE );
    log_debug( "testValue is 0x%04X\n", testValue );
    int result = TEST_ASSERT_EQUAL( expectedLogMessage, logMessage );
    return result;
}
