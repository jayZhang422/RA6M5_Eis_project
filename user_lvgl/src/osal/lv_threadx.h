/**
 * @file lv_threadx.h
 *
 */

/**
 * Copyright 2025 B3W
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LV_THREADX_H
#define LV_THREADX_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_os_private.h"

#if LV_USE_OS == LV_OS_THREADX

#include "tx_api.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    void (*task_function)(void *);                  /**< Application thread function. */
    void * task_arg;                                /**< Arguments for application thread function. */
    void * task_stack;                              /**< ThreadX task stack. */
    UINT task_stack_size;                           /**< ThreadX task stack size in bytes. */
    UINT task_stack_allocated;                      /**< Set to TX_TRUE if task stack was allocated, TX_FALSE otherwise. */
    char task_name[LV_THREADX_TASK_NAME_MAX_LENGTH];/**< ThreadX task name. */
    TX_THREAD task_handle;                          /**< ThreadX task handle. */
} lv_thread_t;

typedef struct {
	UINT mutex_initialized;                         /**< Set to TX_TRUE if mutex is initialized, TX_FALSE otherwise. */
	char* mutex_name;                               /**< ThreadX mutex name. */
	TX_SEMAPHORE mutex_handle;                      /**< ThreadX mutex handle. */
} lv_mutex_t;

typedef struct {
	UINT cond_initialized;                          /**< Set to TX_TRUE if condition variable is initialized, TX_FALSE otherwise. */
    UINT cond_sync_signal;                          /**< Set to TX_TRUE if thread is signaled, TX_FALSE otherwise. */

    TX_SEMAPHORE cond_wait_semaphore;               /**< Threads block on this semaphore in lv_thread_sync_wait. */
    UINT cond_waiting_threads;                      /**< The number of threads currently waiting on this condition variable. */
    lv_mutex_t cond_sync_mutex;                     /**< Threads take this mutex before accessing the condition variable. */
} lv_thread_sync_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_OS == LV_OS_THREADX*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_THREADX_H */
