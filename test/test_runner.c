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
#include <stdbool.h>
#include <stdarg.h>
#include "log_ec.h"


/* Private macro definitions ------------------------------------------------*/

#define EXPECTED_NUMBER_OF_ARGS 2           /* Expected number of command line arguments, including the executable command */
#define DEFAULT_EXPECTED_TIMESTAMP 12345U   /* Default timestamp value that is expected to be included in log message */
#define TEST_BUFFER_SIZE 80U                /* Size in bytes of the test buffer to which log messages are written*/
#define NEXT_LINE ( __LINE__ + 1 )          /* Line number of the next line */

/**
 * @brief Compare a log message string with the expected message string.
 * 
 * @return 0 if the messages are identical, a non-zero value if the messages are different.
 */
#define TEST_ASSERT_EQUAL_STRING( EXPECTED_MESSAGE, MESSAGE ) \
        strcmp( EXPECTED_MESSAGE, MESSAGE )

/**
 * @brief Compare two integers.
 * 
 * @return 0 if the integers are identical, a non-zero value if the integers are different.
 */
#define TEST_ASSERT_EQUAL_INT( EXPECTED_NUM, NUM ) \
        ( EXPECTED_NUM - NUM )

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
static bool setLockState( bool lock, void* lockData );
static void clearLogMessage( void );

static int test_log_trace_messageFormat( void );
static int test_log_debug_messageFormat( void );
static int test_log_info_messageFormat( void );
static int test_log_warn_messageFormat( void );
static int test_log_error_messageFormat( void );
static int test_log_fatal_with10DigitTimestamp_messageFormat( void );
static int test_log_info_withLockFreeShallWriteLogMessage( void );
static int test_log_info_withLockTakenShallNotWriteLogMessage( void );


/* Private variable definitions ---------------------------------------------*/

/** @brief List of unit test functions */
tTestItem m_testList[] = {
    { "log_trace message format", test_log_trace_messageFormat },
    { "log_debug message format", test_log_debug_messageFormat },
    { "log_info message format", test_log_info_messageFormat },
    { "log_warn message format", test_log_warn_messageFormat },
    { "log_error message format", test_log_error_messageFormat },
    { "log_fatal message format with 10 digit timestamp", test_log_fatal_with10DigitTimestamp_messageFormat },
    { "log message shall be written when lock is free", test_log_info_withLockFreeShallWriteLogMessage },
    { "log message shall not be written when lock is taken", test_log_info_withLockTakenShallNotWriteLogMessage }
};

/** Number of test cases */
const size_t m_testListLength = sizeof( m_testList ) / sizeof( m_testList[0U] );

/** Expected timestamp value */
uint32_t m_timestamp = 0U;

/** Test buffer to which log messages are written */
char m_logMessage[TEST_BUFFER_SIZE] = { '\0'};

/** Test buffer write index */
size_t m_logMessageWriteIndex = 0U;

/** Mock mutex state variable */
bool m_logIsLocked = false;

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
        for( size_t i = 0U; ( i < m_testListLength ) && ( !testComplete ); i++ )
        {
            if( 0 == strcmp( testName, m_testList[i].testName ) )
            {
                // test setup
                log_setTimestampFn( getTimestamp );
                setExpectedTimestamp( DEFAULT_EXPECTED_TIMESTAMP );
                clearLogMessage();

                // run the test with the specified funcion name
                result = m_testList[i].testFunction();
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
    size_t freeSpace = TEST_BUFFER_SIZE - m_logMessageWriteIndex;
    int bytesWritten = vsnprintf( &m_logMessage[m_logMessageWriteIndex], freeSpace , format, arg );
    m_logMessageWriteIndex += bytesWritten;
    va_end( arg );
    return bytesWritten;
}

int testVprintf( const char* format, va_list arg)
{
    size_t freeSpace = TEST_BUFFER_SIZE - m_logMessageWriteIndex;
    int bytesWritten = vsnprintf( &m_logMessage[m_logMessageWriteIndex], freeSpace, format, arg );
    m_logMessageWriteIndex += bytesWritten;
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
    m_timestamp = expectedTimestamp;
}

/**
 * @brief Custom timestamp generator function that returns a known timestamp value when running tests.
 * 
 * @return timestamp value
 */
static uint32_t getTimestamp( void )
{
    return m_timestamp;
}


/**
 * @brief Log lock function.
 * 
 * This function mocks a mutex for unit testing. When the lock function is 
 * called with lock=true, a mutex is acquired to prevent any other threads or 
 * RTOS tasks from writing log messages. When the lock function is called with 
 * lock=false, the mutex is released to allow other threads to write log 
 * messages.
 * 
 * @param lock true to acquire the mutex that enables printing of log messages, false to release the mutex.
 * @param lockData Pointer to the mock mutex state variable.
 * 
 * @return true if the mutex was successfully acquired or released, or false on failure.
 */
static bool setLockState( bool lock, void* lockData )
{
    bool* pLocked = lockData;
    bool success = true;
    if( lock )
    {
        /* request to acquire lock */
        success = !(*pLocked);  /* fail to acquire the lock if it is already taken */
        *pLocked = true;
    }
    else
    {
        /* release lock */
        *pLocked = false;
    }
    return success;
}

/**
 * @brief Clear the log message buffer.
 */
static void clearLogMessage( void )
{
    memset( m_logMessage, 0, TEST_BUFFER_SIZE );
    m_logMessageWriteIndex = 0U;
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
    int msgLen = log_trace( "testValue is %d\n", testValue );
    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief Verify the format of log messages printed by log_debug() macro.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_debug_messageFormat( void )
{
    unsigned int testValue = 0xFACE;
    char expectedLogMessage[80] = { '\0'};
    sprintf( expectedLogMessage, "   12345 DEBUG test_runner.c:%u: testValue is 0xFACE\n", NEXT_LINE );
    int msgLen = log_debug( "testValue is 0x%04X\n", testValue );
    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief Verify the format of log messages printed by log_info() macro.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_info_messageFormat( void )
{
    const char* testValue = "\"Hello world!\"";
    char expectedLogMessage[80] = { '\0'};
    sprintf( expectedLogMessage, "   12345 INFO  test_runner.c:%u: testValue is \"Hello world!\"\n", NEXT_LINE );
    int msgLen = log_info( "testValue is %s\n", testValue );
    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief Verify the format of log messages printed by log_warn() macro.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_warn_messageFormat( void )
{
    unsigned int testValue = -2001;
    char expectedLogMessage[80] = { '\0'};
    sprintf( expectedLogMessage, "   12345 WARN  test_runner.c:%u: testValue is -2001\n", NEXT_LINE );
    int msgLen = log_warn( "testValue is %d\n", testValue );
    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief Verify the format of log messages printed by log_error() macro.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_error_messageFormat( void )
{
    int testValue = 48;
    char expectedLogMessage[80] = { '\0'};
    setExpectedTimestamp( 0U );  /* Zero timestamp should be right-justified in the 8-character timestamp field */
    sprintf( expectedLogMessage, "       0 ERROR test_runner.c:%u: testValue is 48\n", NEXT_LINE );
    int msgLen = log_error( "testValue is %d\n", testValue );
    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief Verify the format of log messages printed by log_fatal() macro, with 10 digit timestamp.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_fatal_with10DigitTimestamp_messageFormat( void )
{
    unsigned int testValue = 77U;
    char expectedLogMessage[80] = { '\0'};
    setExpectedTimestamp( 4294967295U );  /* 10 digit timestamp causes width of 8-character timestamp field to increase */
    sprintf( expectedLogMessage, "4294967295 FATAL test_runner.c:%u: testValue is 77\n", NEXT_LINE );
    int msgLen = log_fatal( "testValue is %u\n", testValue );
    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}


/**
 * @brief Verify the that writing of a log_info() message succeeds when the lock 
 *        is free and can be acquired by the lock function.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_info_withLockFreeShallWriteLogMessage( void )
{
    int testValue = 48;
    char expectedLogMessage[80] = { '\0'};
    log_setLockFn( setLockState, &m_logIsLocked );
    sprintf( expectedLogMessage, "   12345 TRACE test_runner.c:%u: testValue is 48\n", NEXT_LINE );
    int msgLen = log_trace( "testValue is %d\n", testValue );
    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}


/**
 * @brief Verify the that writing of a log_info() message fails when the lock 
 *        is taken and cannot be acquired by the lock function.
 * 
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_info_withLockTakenShallNotWriteLogMessage( void )
{
    int testValue = 48;
    char expectedLogMessage[80] = { '\0'};
    int expectedMsgLen = 0;
    log_setLockFn( setLockState, &m_logIsLocked );
    m_logIsLocked = true;  /* Simulate lock acquisition by another thread */
    int msgLen = log_trace( "testValue is %d\n", testValue );
    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );  /* empty message buffer */
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}
