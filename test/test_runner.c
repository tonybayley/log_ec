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
 * @brief Compare two strings.
 *
 * @param EXPECTED_MESSAGE First of the strings to be compared.
 * @param MESSAGE Second of the strings to be compared.
 *
 * @return 0 if the strings are identical, a non-zero value if they are different.
 */
#define TEST_ASSERT_EQUAL_STRING( EXPECTED_MESSAGE, MESSAGE ) \
        strcmp( EXPECTED_MESSAGE, MESSAGE )

/**
 * @brief Compare two integers.
 *
 * @param EXPECTED_NUM First of the integers to be compared.
 * @param NUM Second of the integers to be compared.
 *
 * @return 0 if the integers are identical, a non-zero value if they are different.
 */
#define TEST_ASSERT_EQUAL_INT( EXPECTED_NUM, NUM ) \
        ( (int)( EXPECTED_NUM - NUM ) )

/**
 * @brief Test for NULL pointer.
 *
 * @param EXPR Expression to be tested for NULL.
 *
 * @return 0 if the expression is NULL, 1 if it is not NULL.
 */
#define TEST_ASSERT_NULL( EXPR ) \
        ( NULL == ( EXPR ) ) ? 0 : 1

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

/**
 * @brief Type for storage of data passed to a callback function.
 */
typedef struct {
    tLog_event ev;
    void* data;
    char logMessage[TEST_BUFFER_SIZE];
} tCallbackData;

/* Private function declarations --------------------------------------------*/

static void setExpectedTimestamp( uint32_t expectedTimestamp );
static uint32_t getTimestamp( void );
static bool setLockState( bool lock, void* lockData );
static void clearLogMessage( void );
static void clearCallbackData( void );
static void callbackFunction( tLog_event* ev, void* cbData );
static void altCallbackFunction( tLog_event* ev, void* cbData );

static int test_log_trace_messageFormat( void );
static int test_log_debug_messageFormat( void );
static int test_log_info_messageFormat( void );
static int test_log_warn_messageFormat( void );
static int test_log_error_messageFormat( void );
static int test_log_fatal_with10DigitTimestamp_messageFormat( void );
static int test_log_info_withLockFreeShallWriteLogMessage( void );
static int test_log_info_withLockTakenShallNotWriteLogMessage( void );
static int test_log_off( void );
static int test_log_on( void );
static int test_log_setLevel_equalLevelIsPrinted( void );
static int test_log_setLevel_higherLevelIsPrinted( void );
static int test_log_setLevel_lowerLevelIsNotPrinted( void );
static int test_setTimestamp_null( void );
static int test_callback1_logInfo( void );
static int test_callback1_logWarn( void );
static int test_callback1_logDebug( void );
static int test_twoCallbacksShallBeInvoked( void );
static int test_unregister_callback1( void );
static int test_register_overwrite( void );
static int test_thirdSubcriptionShallFail( void );


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
    { "log message shall not be written when lock is taken", test_log_info_withLockTakenShallNotWriteLogMessage },
    { "log_off shall disable printing of log messages", test_log_off },
    { "log_on shall enable printing of log messages", test_log_on },
    { "log message at level set by log_setLevel shall be printed", test_log_setLevel_equalLevelIsPrinted },
    { "log message at higher level than set by log_setLevel shall be printed", test_log_setLevel_higherLevelIsPrinted },
    { "log message at lower level than set by log_setLevel shall not be printed", test_log_setLevel_lowerLevelIsNotPrinted },
    { "when timestamp function is NULL the timestamp value is 0", test_setTimestamp_null },
    { "when callback1 is subscribed with level LOG_INFO then log_info shall invoke callback1", test_callback1_logInfo },
    { "when callback1 is subscribed with level LOG_INFO then log_warn shall invoke callback1", test_callback1_logWarn },
    { "when callback1 is subscribed with level LOG_INFO then log_debug shall not invoke callback1", test_callback1_logDebug },
    { "when callback1 and callback2 are subscribed both callbacks shall be invoked", test_twoCallbacksShallBeInvoked },
    { "given 2 subscribed callbacks when callback1 is unsubscribed only callback2 shall be invoked", test_unregister_callback1 },
    { "given callback1 is subscribed the subscription shall be overwritten when resubscribed", test_register_overwrite },
    { "given 2 subscribed callbacks an attempt to subscribe a third callback shall fail", test_thirdSubcriptionShallFail }
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

/** callback1 data  */
tCallbackData m_callback1Data;

/** callback2 data  */
tCallbackData m_callback2Data;

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
                clearCallbackData();

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

/**
 * @brief Clear all callback data objects.
 */
static void clearCallbackData( void )
{
    tCallbackData* callbackData[] = { &m_callback1Data, &m_callback2Data };
    for( size_t i = 0U; i < sizeof( callbackData ) / sizeof( callbackData[0U] ); i++ )
    {
        memset( callbackData[i]->logMessage, 0, sizeof( callbackData[i]->logMessage ) );
        callbackData[i]->data = NULL;
        callbackData[i]->ev = (tLog_event) {
            .file = NULL,
            .fmt = NULL,
            .level = 0,
            .line = 0,
            .time = 0U
        };
    }
}

/**
 * @brief Callback function used for tests of logging callbacks.
 *
 * @param ev Pointer to logging event data.
 * @param cbData Pointer to the callback function's registered callback data object.
 */
static void callbackFunction( tLog_event* ev, void* cbData )
{
    /* Get reference to the registered callback data object */
    tCallbackData* callbackData = cbData;

    callbackData->ev = *ev;
    callbackData->data = cbData;
    vsnprintf( callbackData->logMessage, sizeof( callbackData->logMessage ), ev->fmt, ev->ap );
}

/**
 * @brief Alternative callback function used for tests of logging callbacks.
 *
 * @param ev Pointer to logging event data.
 * @param cbData Pointer to the callback function's registered callback data object.
 */
static void altCallbackFunction( tLog_event* ev, void* cbData )
{
    (void) ev;
    (void) cbData;
    /* Do nothing */
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

    // UUT
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

    // UUT
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

    //UUT
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

    // UUT
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

    // UUT
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

    // UUT
    sprintf( expectedLogMessage, "4294967295 FATAL test_runner.c:%u: testValue is 77\n", NEXT_LINE );
    int msgLen = log_fatal( "testValue is %u\n", testValue );

    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}


/**
 * @brief Verify that writing of a log_info() message succeeds when the lock is
 *        free and can be acquired by the lock function.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_info_withLockFreeShallWriteLogMessage( void )
{
    int testValue = 48;
    char expectedLogMessage[80] = { '\0'};
    log_setLockFn( setLockState, &m_logIsLocked );
    m_logIsLocked = false;  /* lock is free */

    // UUT
    sprintf( expectedLogMessage, "   12345 TRACE test_runner.c:%u: testValue is 48\n", NEXT_LINE );
    int msgLen = log_trace( "testValue is %d\n", testValue );

    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}


/**
 * @brief Verify that writing of a log_info() message fails when the lock is
 *        taken and cannot be acquired by the lock function.
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

    // UUT
    int msgLen = log_trace( "testValue is %d\n", testValue );

    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );  /* empty message buffer */
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}


/**
 * @brief Verify that calling log_off() disables writing of log messages to the console.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_off( void )
{
    int testValue = 27;
    char expectedLogMessage[80] = { '\0'};
    int expectedMsgLen = 0;

    // UUT
    log_off();  /* disable printing of log messages to the console */
    int msgLen = log_error( "testValue is %d\n", testValue );

    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );    /* empty message buffer */
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief Verify that calling log_on() enables writing of log messages to the console.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_on( void )
{
    /* setup: call log_off() and verify that printing of log messages is disabled */
    int result = test_log_off();

    int testValue = 93;
    char expectedLogMessage[80] = { '\0'};
    setExpectedTimestamp( 13579U );  /* Zero timestamp should be right-justified in the 8-character timestamp field */

    // UUT
    log_on();
    sprintf( expectedLogMessage, "   13579 ERROR test_runner.c:%u: testValue is 93\n", NEXT_LINE );
    int msgLen = log_error( "testValue is %d\n", testValue );

    result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief After calling log_setLevel( LOG_WARN ), calling log_warn() shall print log messages to the console.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_setLevel_equalLevelIsPrinted( void )
{
    unsigned int testValue = -2001;
    char expectedLogMessage[80] = { '\0'};

    // UUT
    log_setLevel( LOG_WARN );  /* Set the logging level */
    sprintf( expectedLogMessage, "   12345 WARN  test_runner.c:%u: testValue is -2001\n", NEXT_LINE );
    int msgLen = log_warn( "testValue is %d\n", testValue );

    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief After calling log_setLevel( LOG_WARN ), calling log_error() shall print log messages to the console.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_setLevel_higherLevelIsPrinted( void )
{
    int testValue = 48;
    char expectedLogMessage[80] = { '\0'};

    // UUT
    log_setLevel( LOG_WARN );  /* Set the logging level */
    setExpectedTimestamp( 0U );  /* Zero timestamp should be right-justified in the 8-character timestamp field */
    sprintf( expectedLogMessage, "       0 ERROR test_runner.c:%u: testValue is 48\n", NEXT_LINE );
    int msgLen = log_error( "testValue is %d\n", testValue );

    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief After calling log_setLevel( LOG_WARN ), calling log_info() shall not print log messages to the console.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_log_setLevel_lowerLevelIsNotPrinted( void )
{
    char expectedLogMessage[80] = { '\0'};
    int expectedMsgLen = 0;

    // UUT
    log_setLevel( LOG_WARN );  /* Set the logging level */
    int msgLen = log_info( "This message is not expected to be printed\n" );

    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );    /* empty message buffer */
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief When the timestamp function is NULL, timestamp value is 0.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_setTimestamp_null( void )
{
    char expectedLogMessage[80] = { '\0'};

    //UUT
    log_setTimestampFn( NULL );
    sprintf( expectedLogMessage, "       0 INFO  test_runner.c:%u: Message with zero timestamp\n", NEXT_LINE );
    int msgLen = log_info( "Message with zero timestamp\n" );

    int result = TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_logMessage );
    int expectedMsgLen = strlen( expectedLogMessage );
    result |= TEST_ASSERT_EQUAL_INT( expectedMsgLen, msgLen );
    return result;
}

/**
 * @brief When callback1 has been subscribed with level LOG_INFO, then a call
 * to log_info() shall invoke callback1.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_callback1_logInfo( void )
{
    const char* testValue = "\"Hello world!\"";
    char expectedLogMessage[80] = { '\0'};
    sprintf( expectedLogMessage, "testValue is \"Hello world!\"\n" );

    //UUT
    int result = log_registerCallbackFn( callbackFunction, &m_callback1Data, LOG_INFO ) ? 0 : 1;
    log_info( "testValue is %s\n", testValue );

    result |= TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_callback1Data.logMessage );
    result |= TEST_ASSERT_EQUAL_INT( &m_callback1Data, (tCallbackData*)m_callback1Data.data );
    result |= TEST_ASSERT_EQUAL_INT( LOG_INFO, m_callback1Data.ev.level );
    result |= TEST_ASSERT_EQUAL_INT( ( __LINE__ - 5 ), m_callback1Data.ev.line );
    result |= TEST_ASSERT_EQUAL_INT( DEFAULT_EXPECTED_TIMESTAMP, m_callback1Data.ev.time );
    result |= TEST_ASSERT_EQUAL_STRING( "test_runner.c", m_callback1Data.ev.file );
    return result;
}

/**
 * @brief When callback1 has been subscribed with level LOG_INFO, then a call
 * to log_warn() shall invoke callback1.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_callback1_logWarn( void )
{
    int testValue = -256;
    char expectedLogMessage[80] = { '\0'};
    sprintf( expectedLogMessage, "testValue is -256\n" );

    //UUT
    int result = log_registerCallbackFn( callbackFunction, &m_callback1Data, LOG_INFO ) ? 0 : 1;
    log_warn( "testValue is %d\n", testValue );

    result |= TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_callback1Data.logMessage );
    result |= TEST_ASSERT_EQUAL_INT( &m_callback1Data, (tCallbackData*)m_callback1Data.data );
    result |= TEST_ASSERT_EQUAL_INT( LOG_WARN, m_callback1Data.ev.level );
    result |= TEST_ASSERT_EQUAL_INT( ( __LINE__ - 5 ), m_callback1Data.ev.line );
    result |= TEST_ASSERT_EQUAL_INT( DEFAULT_EXPECTED_TIMESTAMP, m_callback1Data.ev.time );
    result |= TEST_ASSERT_EQUAL_STRING( "test_runner.c", m_callback1Data.ev.file );
    return result;
}

/**
 * @brief When callback1 has been subscribed with level LOG_INFO, then a call
 * to log_debug() shall not invoke callback1.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_callback1_logDebug( void )
{
    int testValue = 1024;
    char expectedLogMessage[80] = { '\0'};

    //UUT
    int result = log_registerCallbackFn( callbackFunction, &m_callback1Data, LOG_INFO ) ? 0 : 1;
    log_debug( "testValue is %d\n", testValue );

    /* callback data has not been set */
    result |= TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_callback1Data.logMessage );
    result |= TEST_ASSERT_EQUAL_INT(  (tCallbackData*)NULL, (tCallbackData*)m_callback1Data.data );
    result |= TEST_ASSERT_EQUAL_INT( LOG_TRACE, m_callback1Data.ev.level );
    result |= TEST_ASSERT_EQUAL_INT( 0, m_callback1Data.ev.line );
    result |= TEST_ASSERT_EQUAL_INT( 0U, m_callback1Data.ev.time );
    result |= TEST_ASSERT_NULL( m_callback1Data.ev.file );
    return result;
}

/**
 * @brief When callback1 and callback2 have been subscribed with levels 
 * LOG_INFO and LOG_DEBUG respectively, then a call to log_info() shall invoke
 * both callbacks.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_twoCallbacksShallBeInvoked( void )
{
    const char* testValue = "\"Hello world!\"";
    char expectedLogMessage[80] = { '\0'};
    sprintf( expectedLogMessage, "testValue is \"Hello world!\"\n" );

    //UUT
    int result = log_registerCallbackFn( callbackFunction, &m_callback1Data, LOG_INFO ) ? 0 : 1;
    result |= log_registerCallbackFn( callbackFunction, &m_callback2Data, LOG_DEBUG ) ? 0 : 1;
    log_info( "testValue is %s\n", testValue );

    /* callback1Data has been written by callback function */
    result |= TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_callback1Data.logMessage );
    result |= TEST_ASSERT_EQUAL_INT( &m_callback1Data, (tCallbackData*)m_callback1Data.data );
    result |= TEST_ASSERT_EQUAL_INT( LOG_INFO, m_callback1Data.ev.level );
    result |= TEST_ASSERT_EQUAL_INT( ( __LINE__ - 6 ), m_callback1Data.ev.line );
    result |= TEST_ASSERT_EQUAL_INT( DEFAULT_EXPECTED_TIMESTAMP, m_callback1Data.ev.time );
    result |= TEST_ASSERT_EQUAL_STRING( "test_runner.c", m_callback1Data.ev.file );

    /* callback2Data has been written by callback function */
    result |= TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_callback2Data.logMessage );
    result |= TEST_ASSERT_EQUAL_INT( &m_callback2Data, (tCallbackData*)m_callback2Data.data );
    result |= TEST_ASSERT_EQUAL_INT( LOG_INFO, m_callback2Data.ev.level );
    result |= TEST_ASSERT_EQUAL_INT( ( __LINE__ - 14 ), m_callback2Data.ev.line );
    result |= TEST_ASSERT_EQUAL_INT( DEFAULT_EXPECTED_TIMESTAMP, m_callback2Data.ev.time );
    result |= TEST_ASSERT_EQUAL_STRING( "test_runner.c", m_callback2Data.ev.file );
    return result;
}

/**
 * @brief Given that callback1 and callback2 have been subscribed with levels 
 * LOG_INFO and LOG_DEBUG respectively, and then callback1 has been unregistered,
 * then a call to log_info() shall invoke callback2 only.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_unregister_callback1( void )
{
    /* given that callback1 and callback2 have been subscribed */
    int result = test_twoCallbacksShallBeInvoked();

    clearLogMessage();
    clearCallbackData();

    const char* testValue = "\"Hello world!\"";
    char expectedLog2Message[80] = { '\0'};
    sprintf( expectedLog2Message, "testValue is \"Hello world!\"\n" );

    //UUT
    log_unregisterCallbackFn( callbackFunction, &m_callback1Data );
    log_info( "testValue is %s\n", testValue );

    /* callback1Data has not been set */
    result |= TEST_ASSERT_EQUAL_STRING( "", m_callback1Data.logMessage );
    result |= TEST_ASSERT_EQUAL_INT(  (tCallbackData*)NULL, (tCallbackData*)m_callback1Data.data );
    result |= TEST_ASSERT_EQUAL_INT( LOG_TRACE, m_callback1Data.ev.level );
    result |= TEST_ASSERT_EQUAL_INT( 0, m_callback1Data.ev.line );
    result |= TEST_ASSERT_EQUAL_INT( 0U, m_callback1Data.ev.time );
    result |= TEST_ASSERT_NULL( m_callback1Data.ev.file );

    /* callback2Data has been written by callback function */
    result |= TEST_ASSERT_EQUAL_STRING( expectedLog2Message, m_callback2Data.logMessage );
    result |= TEST_ASSERT_EQUAL_INT( &m_callback2Data, (tCallbackData*)m_callback2Data.data );
    result |= TEST_ASSERT_EQUAL_INT( LOG_INFO, m_callback2Data.ev.level );
    result |= TEST_ASSERT_EQUAL_INT( ( __LINE__ - 14 ), m_callback2Data.ev.line );
    result |= TEST_ASSERT_EQUAL_INT( DEFAULT_EXPECTED_TIMESTAMP, m_callback2Data.ev.time );
    result |= TEST_ASSERT_EQUAL_STRING( "test_runner.c", m_callback2Data.ev.file );
    return result;
}

/**
 * @brief Given that callback1 has been subscribed, subsequent re-subscription
 * of the same callback function and data object shall overwrite the original
 * subscription such that there is still space for subscription of callback2 to
 * succeed.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_register_overwrite( void )
{
    /* Given that callback1 has been subscribed with logging level = LOG_INFO */
    int result = test_callback1_logInfo();

    clearLogMessage();
    clearCallbackData();

    const char* testValue = "\"Hello world!\"";
    char expectedLogMessage[80] = { '\0'};
    sprintf( expectedLogMessage, "testValue is \"Hello world!\"\n" );

    //UUT
    result |= log_registerCallbackFn( callbackFunction, &m_callback1Data, LOG_DEBUG ) ? 0 : 1;
    result |= log_registerCallbackFn( callbackFunction, &m_callback2Data, LOG_DEBUG ) ? 0 : 1;

    /* Verify that the registered logging level for callback 1 has been reduced from LOG_INFO to LOG_DEBUG */
    log_debug( "testValue is %s\n", testValue );
    result |= TEST_ASSERT_EQUAL_STRING( expectedLogMessage, m_callback1Data.logMessage );
    result |= TEST_ASSERT_EQUAL_INT( &m_callback1Data, (tCallbackData*)m_callback1Data.data );
    result |= TEST_ASSERT_EQUAL_INT( LOG_DEBUG, m_callback1Data.ev.level );
    result |= TEST_ASSERT_EQUAL_INT( ( __LINE__ - 4 ), m_callback1Data.ev.line );
    result |= TEST_ASSERT_EQUAL_INT( DEFAULT_EXPECTED_TIMESTAMP, m_callback1Data.ev.time );
    result |= TEST_ASSERT_EQUAL_STRING( "test_runner.c", m_callback1Data.ev.file );
    return result;
}

/**
 * @brief Given that callback1 and callback2 have been subscribed, an attempt to
 * register a third callback shall fail.
 *
 * @return 0 if test passes, 1 if test fails. 
 */
static int test_thirdSubcriptionShallFail( void )
{
    /* Given that 2 callbacks have been subscribed */
    int result = test_twoCallbacksShallBeInvoked();

    /* attempt to register one of the same callbacks with a different object shall fail */
    result |= log_registerCallbackFn( callbackFunction, &result, LOG_INFO ) ? 1 : 0;

    /* attempt to register a different callback shall fail */
    result |= log_registerCallbackFn( altCallbackFunction, &m_callback1Data, LOG_WARN ) ? 1 : 0;
    return result;
}
