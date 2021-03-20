/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "params.h"
#include "queue.h"

volatile SemaphoreHandle_t bouton_semph = 0;
volatile bool estAppuye = false;
volatile task_params_t task_A = {
    .delay = 1000,
    .message = "Tache A en cours\n\r"
};
volatile task_params_t task_B = {
    .delay = 900,
    .message = "Tache B en cours\n\r"
};
volatile QueueHandle_t print_queue;

void LED_task()
{
    for (;;)
    {
        Cy_GPIO_Write(LED_0_PORT, LED_0_NUM,0);
        vTaskDelay(pdMS_TO_TICKS(500));
        Cy_GPIO_Write(LED_0_PORT, LED_0_NUM,1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void isr_bouton()
{
    xSemaphoreGiveFromISR(bouton_semph, NULL);
    Cy_GPIO_ClearInterrupt(BOUTON_0_PORT, BOUTON_0_NUM);
    NVIC_ClearPendingIRQ(Bouton_ISR_cfg.intrSrc);
}

void bouton_task()
{
    for (;;)
    {
        if (xSemaphoreTake(bouton_semph,500))
        {
            vTaskDelay(pdMS_TO_TICKS(20));
            if (estAppuye)
            {
                estAppuye = false;
                UART_PutString("Bouton relache\n\r");
            }
            else
            {
                estAppuye = true;
                UART_PutString("Bouton appuye\n\r");
            }
        }
    }
}

void print_loop(void * params)
{
    task_params_t * tache_en_cours = params;
    for (;;)
    {
        vTaskDelay(tache_en_cours->delay);
        UART_PutString(tache_en_cours->message);
        //xQueueSendToBack(print_queue, tache_en_cours->message, tache_en_cours->delay);
    }
}

//void print()
//{
    //char * message;
    //for(;;)
    //{
        //xQueueReceive(print_queue, &message, portMAX_DELAY);
        //UART_PutString(message);
    //}
//}

int main(void)
{
    __enable_irq(); /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    xTaskCreate(LED_task, "LED", 500, NULL, 2, NULL);
    bouton_semph = xSemaphoreCreateBinary();
    print_queue = xQueueCreate(2, sizeof(char *));
    
    UART_Start();
    
    Cy_SysInt_Init(&Bouton_ISR_cfg, isr_bouton);
    NVIC_ClearPendingIRQ(Bouton_ISR_cfg.intrSrc);
    NVIC_EnableIRQ(Bouton_ISR_cfg.intrSrc);
    
    xTaskCreate(bouton_task, "BOUTON", 500, NULL, 2, NULL);
    
    xTaskCreate(print_loop, "task A", configMINIMAL_STACK_SIZE, (void *) &task_A, 1, NULL);
    xTaskCreate(print_loop, "task B", configMINIMAL_STACK_SIZE, (void *) &task_B, 1, NULL);
    //xTaskCreate(print, "print", configMINIMAL_STACK_SIZE, NULL, 0, NULL);
    
    vTaskStartScheduler();

    for(;;)
    {
        /* Place your application code here. */
    }
}

/* [] END OF FILE */
