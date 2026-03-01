/* generated thread source file - do not edit */
#include "App_Thread.h"

TX_THREAD App_Thread;
                void App_Thread_create(void);
                static void App_Thread_func(ULONG thread_input);
                static uint8_t App_Thread_stack[1024] BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".stack.App_Thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
                void tx_startup_err_callback(void * p_instance, void * p_data);
                void tx_startup_common_init(void);
extern bool         g_fsp_common_initialized;
                extern uint32_t     g_fsp_common_thread_count;
                extern TX_SEMAPHORE g_fsp_common_initialized_semaphore;

                void App_Thread_create (void)
                {
                    /* Increment count so we will know the number of ISDE created threads. */
                    g_fsp_common_thread_count++;

                    /* Initialize each kernel object. */
                    

                    UINT err;
                    err = tx_thread_create(
                        &App_Thread,
                        (CHAR *)"App_Thread",
                        App_Thread_func,
                        (ULONG) NULL,
                        &App_Thread_stack,
                        1024,
                        1,
                        1,
                        1,
                        TX_AUTO_START
                    );
                    if (TX_SUCCESS != err) {
                        tx_startup_err_callback(&App_Thread, 0);
                    }
                }

                static void App_Thread_func (ULONG thread_input) {
                    /* Not currently using thread_input. */
                    FSP_PARAMETER_NOT_USED(thread_input);

                    /* Initialize common components */
                    tx_startup_common_init();

                    /* Initialize each module instance. */
                    

                    /* Enter user code for this thread. */
                    App_Thread_entry();
                }
