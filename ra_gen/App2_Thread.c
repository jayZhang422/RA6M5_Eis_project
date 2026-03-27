/* generated thread source file - do not edit */
#include "App2_Thread.h"

TX_THREAD App2_Thread;
                void App2_Thread_create(void);
                static void App2_Thread_func(ULONG thread_input);
                static uint8_t App2_Thread_stack[4096] BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".stack.App2_Thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
                void tx_startup_err_callback(void * p_instance, void * p_data);
                void tx_startup_common_init(void);
extern bool         g_fsp_common_initialized;
                extern uint32_t     g_fsp_common_thread_count;
                extern TX_SEMAPHORE g_fsp_common_initialized_semaphore;

                void App2_Thread_create (void)
                {
                    /* Increment count so we will know the number of ISDE created threads. */
                    g_fsp_common_thread_count++;

                    /* Initialize each kernel object. */
                    

                    UINT err;
                    err = tx_thread_create(
                        &App2_Thread,
                        (CHAR *)"App2_Thread",
                        App2_Thread_func,
                        (ULONG) NULL,
                        &App2_Thread_stack,
                        4096,
                        1,
                        1,
                        1,
                        TX_AUTO_START
                    );
                    if (TX_SUCCESS != err) {
                        tx_startup_err_callback(&App2_Thread, 0);
                    }
                }

                static void App2_Thread_func (ULONG thread_input) {
                    /* Not currently using thread_input. */
                    FSP_PARAMETER_NOT_USED(thread_input);

                    /* Initialize common components */
                    tx_startup_common_init();

                    /* Initialize each module instance. */
                    

                    /* Enter user code for this thread. */
                    App2_Thread_entry();
                }
