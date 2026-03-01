/* generated thread source file - do not edit */
#include "Idle_Thread.h"

TX_THREAD Idle_Thread;
                void Idle_Thread_create(void);
                static void Idle_Thread_func(ULONG thread_input);
                static uint8_t Idle_Thread_stack[1024] BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".stack.Idle_Thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
                void tx_startup_err_callback(void * p_instance, void * p_data);
                void tx_startup_common_init(void);
extern bool         g_fsp_common_initialized;
                extern uint32_t     g_fsp_common_thread_count;
                extern TX_SEMAPHORE g_fsp_common_initialized_semaphore;

                void Idle_Thread_create (void)
                {
                    /* Increment count so we will know the number of ISDE created threads. */
                    g_fsp_common_thread_count++;

                    /* Initialize each kernel object. */
                    

                    UINT err;
                    err = tx_thread_create(
                        &Idle_Thread,
                        (CHAR *)"Idle_Thread",
                        Idle_Thread_func,
                        (ULONG) NULL,
                        &Idle_Thread_stack,
                        1024,
                        31,
                        31,
                        1,
                        TX_AUTO_START
                    );
                    if (TX_SUCCESS != err) {
                        tx_startup_err_callback(&Idle_Thread, 0);
                    }
                }

                static void Idle_Thread_func (ULONG thread_input) {
                    /* Not currently using thread_input. */
                    FSP_PARAMETER_NOT_USED(thread_input);

                    /* Initialize common components */
                    tx_startup_common_init();

                    /* Initialize each module instance. */
                    

                    /* Enter user code for this thread. */
                    Idle_Thread_entry();
                }
