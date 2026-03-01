/**
 * @file lv_threadx.c
 *
 */

/**
 * Copyright 2025 B3W
 *
 * SPDX-License-Identifier: MIT
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_os_private.h"
#if LV_USE_OS == LV_OS_THREADX

#ifdef TX_INCLUDE_USER_DEFINE_FILE
#include "tx_user.h"
#endif

#include "../misc/lv_log.h"
#include "../misc/lv_math.h"
#include "../misc/lv_timer.h"
#include "../stdlib/lv_string.h"

/*********************
 *      DEFINES
 *********************/

/* Allocate memory pool such that it can be aligned as needed */
#define THREADX_BYTE_POOL_SIZE (LV_THREADX_TASK_MEMORY_POOL_SIZE + (LV_THREADX_TASK_MEMORY_POOL_ALIGN - 1))

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    UINT byte_pool_initialized;                     /**< Set to TX_TRUE if byte pool initialized, TX_FALSE otherwise */
    UCHAR byte_pool_buffer[THREADX_BYTE_POOL_SIZE]; /**< Backing memory for ThreadX byte pool */
    UCHAR * byte_pool_buffer_address;               /**< Aligned start address for byte pool's backing memory */
    TX_BYTE_POOL byte_pool_handle;                  /**< Handle to ThreadX byte pool */
} lv_byte_pool_wrapper_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void run_task(ULONG arg);

static UINT byte_pool_wrapper_init(lv_byte_pool_wrapper_t * byte_pool_wrapper);

static UINT check_byte_pool_wrapper_init(lv_byte_pool_wrapper_t * byte_pool_wrapper);

static UINT enter_critical_section(void);

static void exit_critical_section(UINT old_posture);

/**********************
 *  STATIC VARIABLES
 **********************/

/* Byte pool which ThreadX task stacks will be allocated out of */
static lv_byte_pool_wrapper_t task_byte_pool = {0};

/* Priority mapping for ThreadX thread priorities */
static const UINT thread_priority_map[] = {
    [LV_THREAD_PRIO_LOWEST] = 5U,
    [LV_THREAD_PRIO_LOW] = 4U,
    [LV_THREAD_PRIO_MID] = 3U,
    [LV_THREAD_PRIO_HIGH] = 2U,
    [LV_THREAD_PRIO_HIGHEST] = 1U,
};

/* Name of semaphores associated with sync objects */
static char* SYNC_SEM_NAME = "SyncSem";

/* Name of mutexes associated with sync objects */
static char* SYNC_MUTEX_NAME = "SyncMutex";

/**********************
 *      MACROS
 **********************/

/**********************
 *  GLOBAL FUNCTIONS
 **********************/

lv_result_t lv_thread_init(lv_thread_t * thread,
                           const char * const name,
                           lv_thread_prio_t prio,
                           void (*callback)(void *),
                           size_t stack_size,
                           void * user_data)
{
    /* Initialize the task byte pool if needed */
    UINT status = check_byte_pool_wrapper_init(&task_byte_pool);

    if(status != TX_SUCCESS) {
        return LV_RESULT_INVALID;
    }

    lv_result_t result = LV_RESULT_OK;
    thread->task_arg = user_data;
    thread->task_function = callback;

    /* Copy the task name and ensure null termination */
    thread->task_name[0] = '\0';

    if(name != NULL) {
        lv_strlcpy(thread->task_name, name, LV_THREADX_TASK_NAME_MAX_LENGTH);
    }

    /* If no stack passed in, allocate stack memory out of ThreadX byte pool */
    if(thread->task_stack == NULL) {
        size_t aligned_stack_size = stack_size + (stack_size % LV_THREADX_TASK_MEMORY_POOL_ALIGN);
        thread->task_stack_size = aligned_stack_size;

        status = tx_byte_allocate(&task_byte_pool.byte_pool_handle,
                                  &thread->task_stack,
                                  thread->task_stack_size,
                                  TX_NO_WAIT);

        if(status == TX_SUCCESS) {
            LV_LOG_INFO("Allocated %d byte stack for task '%s'", thread->task_stack_size, thread->task_name);
            thread->task_stack_allocated = TX_TRUE;
        }
        else {
            LV_LOG_ERROR("tx_byte_allocate failed with error '%d'", status);
            result = LV_RESULT_INVALID;
        }
    }
    else {
        LV_LOG_INFO("Using user provided stack for task '%s'", thread->task_name);
        thread->task_stack_allocated = TX_FALSE;
        thread->task_stack_size = stack_size;
    }

    /* Create the ThreadX task */
    if(result == LV_RESULT_OK) {
        status = tx_thread_create(&thread->task_handle,
                                  thread->task_name,
                                  run_task,
                                  (ULONG)thread,
                                  thread->task_stack,
                                  thread->task_stack_size,
                                  // Disable time slicing and preemption thresholding
                                  thread_priority_map[prio],
                                  thread_priority_map[prio],
                                  TX_NO_TIME_SLICE,
                                  TX_AUTO_START);

        if(status != TX_SUCCESS) {
            LV_LOG_ERROR("tx_thread_create failed with error '%d'", status);
            result = LV_RESULT_INVALID;
        }
    }

    return result;
}

lv_result_t lv_thread_delete(lv_thread_t * thread)
{
    UINT status = tx_thread_delete(&thread->task_handle);

    if(status != TX_SUCCESS) {
        LV_LOG_ERROR("tx_thread_delete failed with error '%d'", status);
        return LV_RESULT_INVALID;
    }

    /* If memory was allocated out of byte pool for task stack release it */
    lv_result_t result = LV_RESULT_OK;

    if(thread->task_stack_allocated == TX_TRUE) {
        status = tx_byte_release(thread->task_stack);

        if(status == TX_SUCCESS) {
            thread->task_stack_allocated = TX_FALSE;
        }
        else {
            LV_LOG_ERROR("tx_byte_release failed with error '%d'", status);
            result = LV_RESULT_INVALID;
        }
    }

    return result;
}

lv_result_t lv_mutex_init(lv_mutex_t * mutex)
{
    if(mutex->mutex_initialized == TX_TRUE) {
        return LV_RESULT_OK;
    }

    const UINT INITIAL_COUNT_MUTEX = 1U;
    lv_result_t result = LV_RESULT_OK;

    UINT status = tx_semaphore_create(&mutex->mutex_handle, mutex->mutex_name, INITIAL_COUNT_MUTEX);

    if(status == TX_SUCCESS) {
        mutex->mutex_initialized = TX_TRUE;
    }
    else {
        LV_LOG_ERROR("tx_semaphore_create failed with error '%d'", status);
        result = LV_RESULT_INVALID;
    }

    return result;
}

lv_result_t lv_mutex_lock(lv_mutex_t * mutex)
{
    if(mutex->mutex_initialized == TX_FALSE) {
        return LV_RESULT_INVALID;
    }

    lv_result_t result = LV_RESULT_OK;
    UINT status = tx_semaphore_get(&mutex->mutex_handle, TX_WAIT_FOREVER);

    if(status != TX_SUCCESS) {
        LV_LOG_ERROR("tx_semaphore_get failed with error '%d'", status);
        result = LV_RESULT_INVALID;
    }

    return result;
}

lv_result_t lv_mutex_lock_isr(lv_mutex_t * mutex)
{
    if(mutex->mutex_initialized == TX_FALSE) {
        return LV_RESULT_INVALID;
    }

    lv_result_t result = LV_RESULT_OK;
    UINT status = tx_semaphore_get(&mutex->mutex_handle, TX_NO_WAIT);

    if(status != TX_SUCCESS) {
        LV_LOG_ERROR("tx_semaphore_get failed with error '%d'", status);
        result = LV_RESULT_INVALID;
    }

    return result;
}

lv_result_t lv_mutex_unlock(lv_mutex_t * mutex)
{
    if(mutex->mutex_initialized == TX_FALSE) {
        return LV_RESULT_INVALID;
    }

    lv_result_t result = LV_RESULT_OK;
    UINT status = tx_semaphore_put(&mutex->mutex_handle);

    if(status != TX_SUCCESS) {
        LV_LOG_ERROR("tx_semaphore_put failed with error '%d'", status);
        result = LV_RESULT_INVALID;
    }

    return result;
}

lv_result_t lv_mutex_delete(lv_mutex_t * mutex)
{
    if(mutex->mutex_initialized == TX_FALSE) {
        return LV_RESULT_OK;
    }

    UINT status = tx_semaphore_delete(&mutex->mutex_handle);

    if(status != TX_SUCCESS) {
        LV_LOG_ERROR("tx_semaphore_delete failed with error '%d'", status);
    }

    mutex->mutex_initialized = TX_FALSE;

    return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_init(lv_thread_sync_t * sync)
{
    if(sync->cond_initialized == TX_TRUE) {
        return LV_RESULT_OK;
    }

    const UINT INITIAL_COUNT_SEM = 0U;
    lv_result_t result = LV_RESULT_OK;

    UINT status = tx_semaphore_create(&sync->cond_wait_semaphore, SYNC_SEM_NAME, INITIAL_COUNT_SEM);

    if(status != TX_SUCCESS) {
        LV_LOG_ERROR("tx_semaphore_create failed with error '%d'", status);
        result = LV_RESULT_INVALID;
    }

    if (result == LV_RESULT_OK) {
        sync->cond_sync_mutex.mutex_name = SYNC_MUTEX_NAME;
        result = lv_mutex_init(&sync->cond_sync_mutex);

        if(result != LV_RESULT_OK) {
            /* Have to cleanup the semaphore successfully created above */
            tx_semaphore_delete(&sync->cond_wait_semaphore);
        }
    }

    if (result == LV_RESULT_OK) {
        sync->cond_sync_signal = TX_FALSE;
        sync->cond_waiting_threads = 0U;
        sync->cond_initialized = TX_TRUE;
    }

    return result;
}

lv_result_t lv_thread_sync_wait(lv_thread_sync_t * sync)
{
    if(sync->cond_initialized == TX_FALSE) {
        return LV_RESULT_INVALID;
    }

    /* Acquire the mutex prior to waiting */
    lv_result_t result = lv_mutex_lock(&sync->cond_sync_mutex);

    while((result == LV_RESULT_OK) && !sync->cond_sync_signal) {
        /* Increase thread counter then release the mutex and wait */
        sync->cond_waiting_threads++;
        result = lv_mutex_unlock(&sync->cond_sync_mutex);

        if(result == LV_RESULT_OK) {
            /* Wait on condition variable to be signaled */
            UINT status = tx_semaphore_get(&sync->cond_wait_semaphore, TX_WAIT_FOREVER);

            if(status == TX_SUCCESS) {
                result = lv_mutex_lock(&sync->cond_sync_mutex);
            }
            else {
                LV_LOG_ERROR("tx_semaphore_get(cond_wait_semaphore) failed with error '%d'", status);
                result = LV_RESULT_INVALID;
            }
        }
        else {
            LV_LOG_ERROR("lv_mutex_unlock(cond_sync_mutex) failed with error '%d'", status);
        }

        sync->cond_waiting_threads--;           /* Decremented by waiter after wake-up or error */
    }

    if(result == LV_RESULT_OK) {
        /* Reset sync signal after we have consumed it */
        sync->cond_sync_signal = TX_FALSE;

        /* Release our lock so other threads can be unblocked */
        result = lv_mutex_unlock(&sync->cond_sync_mutex);
    }

    return result;
}

lv_result_t lv_thread_sync_signal(lv_thread_sync_t * sync)
{
    if(sync->cond_initialized == TX_FALSE) {
        return LV_RESULT_INVALID;
    }

    /* Acquire the mutex prior to signaling */
    lv_result_t result = lv_mutex_lock(&sync->cond_sync_mutex);

    if(result == LV_RESULT_OK) {
        sync->cond_sync_signal = TX_TRUE;

        /* Unblock all waiting threads */
        for(UINT i = 0U; i < sync->cond_waiting_threads; i++) {
            UINT status = tx_semaphore_put(&sync->cond_wait_semaphore);

            if(status != TX_SUCCESS) {
                LV_LOG_ERROR("tx_semaphore_put(cond_wait_semaphore) failed with error '%d'", status);
            }
        }

        /* Release our lock so other threads can be unblocked */
        result = lv_mutex_unlock(&sync->cond_sync_mutex);
    }

    return result;
}

lv_result_t lv_thread_sync_signal_isr(lv_thread_sync_t * sync)
{
    if(sync->cond_initialized == TX_FALSE) {
        return LV_RESULT_INVALID;
    }

    /* Acquire the mutex prior to signaling */
    lv_result_t result = lv_mutex_lock_isr(&sync->cond_sync_mutex);

    if(result == LV_RESULT_OK) {
        sync->cond_sync_signal = TX_TRUE;

        /* Unblock all waiting threads */
        for(UINT i = 0U; i < sync->cond_waiting_threads; i++) {
            UINT status = tx_semaphore_put(&sync->cond_wait_semaphore);

            if(status != TX_SUCCESS) {
                LV_LOG_ERROR("tx_semaphore_put(cond_wait_semaphore) failed with error '%d'", status);
            }
        }

        /* Release our lock so other threads can be unblocked */
        result = lv_mutex_unlock(&sync->cond_sync_mutex);
    }

    return result;
}

lv_result_t lv_thread_sync_delete(lv_thread_sync_t * sync)
{
    if(sync->cond_initialized == TX_FALSE) {
        return LV_RESULT_OK;
    }

    UINT status = tx_semaphore_delete(&sync->cond_wait_semaphore);

    if(status != TX_SUCCESS) {
        LV_LOG_ERROR("tx_semaphore_delete(cond_wait_semaphore) failed with error '%d'", status);
    }

    lv_result_t result = lv_mutex_delete(&sync->cond_sync_mutex);

    if(result != LV_RESULT_OK) {
        LV_LOG_ERROR("lv_mutex_delete(cond_sync_mutex) failed with error '%d'", status);
    }

    sync->cond_waiting_threads = 0U;
    sync->cond_sync_signal = TX_FALSE;
    sync->cond_initialized = TX_FALSE;

    return LV_RESULT_OK;
}

uint32_t lv_os_get_idle_percent(void)
{
    return lv_timer_get_idle();
}

void lv_sleep_ms(uint32_t ms)
{
    uint32_t sleep_ticks = (uint32_t)((ms * TX_TIMER_TICKS_PER_SECOND) / 1000U);
    tx_thread_sleep(sleep_ticks);
}

/**********************
 *  STATIC FUNCTIONS
 **********************/

/**
 * Run task's mainloop. This wrapper function allows task argument to be passed in.
 * @param arg                 ThreadX task to run mainloop for
 */
static void run_task(ULONG arg)
{
    lv_thread_t * thread = (lv_thread_t *)arg;
    thread->task_function(thread->task_arg);
}

/**
 * Initialize byte pool wrapper
 * @param byte_pool_wrapper   Byte pool wrapper to initialize
 * @return                    TX_SUCCESS: success; Otherwise: failure
 */
static UINT byte_pool_wrapper_init(lv_byte_pool_wrapper_t * byte_pool_wrapper)
{
    /* Calculate aligned start address for the byte pool backing memory */
    byte_pool_wrapper->byte_pool_buffer_address = byte_pool_wrapper->byte_pool_buffer;
    byte_pool_wrapper->byte_pool_buffer_address = (UCHAR *)LV_ALIGN_UP((lv_uintptr_t)byte_pool_wrapper->byte_pool_buffer_address,
                                                                       LV_THREADX_TASK_MEMORY_POOL_ALIGN);

    UINT status = tx_byte_pool_create(&byte_pool_wrapper->byte_pool_handle,
                                      "LVGLThreadXTaskMemoryPool",
                                      byte_pool_wrapper->byte_pool_buffer_address,
                                      THREADX_BYTE_POOL_SIZE);

    if(status == TX_SUCCESS) {
        byte_pool_wrapper->byte_pool_initialized = TX_TRUE;
    }
    else {
        LV_LOG_ERROR("tx_byte_pool_create failed with error '%d'", status);
    }

    return status;
}

/**
 * Check if byte pool wrapper is initialized. If not, perform initialization.
 * This function is thread-safe
 * @param byte_pool_wrapper   Byte pool wrapper to initialize
 * @return                    TX_SUCCESS: success; Otherwise: failure
 */
static UINT check_byte_pool_wrapper_init(lv_byte_pool_wrapper_t * byte_pool_wrapper)
{
    UINT status = TX_SUCCESS;

    if(byte_pool_wrapper->byte_pool_initialized == TX_FALSE) {
        /* Byte pool initialization must be inside a critical section */
        UINT old_posture = enter_critical_section();

        /* Ensure byte pool wasn't initialized while we were waiting */
        if(byte_pool_wrapper->byte_pool_initialized == TX_FALSE) {
            status = byte_pool_wrapper_init(byte_pool_wrapper);
        }

        exit_critical_section(old_posture);
    }

    return status;
}

/**
 * Enter critical section by disabling interrupts
 * @return                    Posture prior to disabling of interrupts
 */
static inline UINT enter_critical_section(void)
{
    return tx_interrupt_control(TX_INT_DISABLE);
}

/**
 * Exit critical section by restoring interrupts
 * @param old_posture         Posture prior to disabling of interrupts
 */
static inline void exit_critical_section(UINT old_posture)
{
    tx_interrupt_control(old_posture);
}

#endif /* LV_USE_OS == LV_OS_THREADX */
