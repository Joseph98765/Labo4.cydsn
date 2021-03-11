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

int main(void)
{
    __enable_irq(); /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    xTaskCreate(LED_task, "LED", 500, NULL, 1, NULL);
    bouton_semph = xSemaphoreCreateBinary();
    
    UART_Start();
    
    Cy_SysInt_Init(&Bouton_ISR_cfg, isr_bouton);
    NVIC_ClearPendingIRQ(Bouton_ISR_cfg.intrSrc);
    NVIC_EnableIRQ(Bouton_ISR_cfg.intrSrc);
    
    xTaskCreate(bouton_task, "BOUTON", 500, NULL, 2, NULL);
    vTaskStartScheduler();

    for(;;)
    {
        /* Place your application code here. */
    }
}

/* [] END OF FILE */
