/* generated thread source file - do not edit */
#include "Lvgl_Thread.h"

TX_THREAD Lvgl_Thread;
                void Lvgl_Thread_create(void);
                static void Lvgl_Thread_func(ULONG thread_input);
                static uint8_t Lvgl_Thread_stack[8192] BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".stack.Lvgl_Thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
                void tx_startup_err_callback(void * p_instance, void * p_data);
                void tx_startup_common_init(void);
extern bool         g_fsp_common_initialized;
                extern uint32_t     g_fsp_common_thread_count;
                extern TX_SEMAPHORE g_fsp_common_initialized_semaphore;

                void Lvgl_Thread_create (void)
                {
                    /* Increment count so we will know the number of ISDE created threads. */
                    g_fsp_common_thread_count++;

                    /* Initialize each kernel object. */
                    

                    UINT err;
                    err = tx_thread_create(
                        &Lvgl_Thread,
                        (CHAR *)"Lvgl_Thread",
                        Lvgl_Thread_func,
                        (ULONG) NULL,
                        &Lvgl_Thread_stack,
                        8192,
                        1,
                        1,
                        1,
                        TX_AUTO_START
                    );
                    if (TX_SUCCESS != err) {
                        tx_startup_err_callback(&Lvgl_Thread, 0);
                    }
                }

                static void Lvgl_Thread_func (ULONG thread_input) {
                    /* Not currently using thread_input. */
                    FSP_PARAMETER_NOT_USED(thread_input);

                    /* Initialize common components */
                    tx_startup_common_init();

                    /* Initialize each module instance. */
                    

                    /* Enter user code for this thread. */
                    Lvgl_Thread_entry();
                }
