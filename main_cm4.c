/******************************************************************************
* File Name: main_cm4.c
*
* Description: Rapport de Laboratoire 4
* Authors : Joe Saade (2027226) et Louis Joseph Laberge (2021962)
* Date : 22 mars 2021
*
* Lien GitHub : https://github.com/Joseph98765/Labo4.cydsn.git
********************************************************************************/
#include "project.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "params.h"
#include "queue.h"

// Sémaphore
volatile SemaphoreHandle_t bouton_semph = 0;
// Booléen qui assure l'aternance entre appuyé et relâché
volatile bool estAppuye = false;
// Task A et B (code donné dans l'énoncé)
volatile task_params_t task_A = {
    .delay = 1000,
    .message = "Tache A en cours\n\r"
};
volatile task_params_t task_B = {
    .delay = 999,
    .message = "Tache B en cours\n\r"
};
// Queue handle
volatile QueueHandle_t print_queue;

/*******************************************************************************
* Function Name: void LED_task()
********************************************************************************
*
* Summary: Tâche qui éteint et allume la LED du port 0 
* avec 500ms de délai entre chaque opération
*
* Parameters:
*  None
*
* Return:
*  None
*
*
*******************************************************************************/
void LED_task(void)
{
    for (;;)
    {
        Cy_GPIO_Write(LED_0_PORT, LED_0_NUM,0);
        vTaskDelay(pdMS_TO_TICKS(500));
        Cy_GPIO_Write(LED_0_PORT, LED_0_NUM,1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
/*******************************************************************************
* Function Name: void isr_bouton()
********************************************************************************
*
* Summary: ISR qui rend le sémaphore disponible
*
* Parameters:
*  None
*
* Return:
*  None
*
*
*******************************************************************************/
void isr_bouton(void)
{
    xSemaphoreGiveFromISR(bouton_semph, NULL);
    Cy_GPIO_ClearInterrupt(BOUTON_0_PORT, BOUTON_0_NUM);
    NVIC_ClearPendingIRQ(Bouton_ISR_cfg.intrSrc);
}
/*******************************************************************************
* Function Name: void bouton_task()
********************************************************************************
*
* Summary: Tâche qui affiche sur le module UART lorsque le bouton est appuyé et lorsqu'il est
* relâché. Il exécute ces fonctions seulement lorsqu'il réussit à prendre le sémaphore.
*
* Parameters:
*  None
*
* Return:
*  None
*
*
*******************************************************************************/
void bouton_task(void)
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
/*******************************************************************************
* Function Name: void print_loop(void * params)
********************************************************************************
*
* Summary: Tâche qui au départ affichait continuellement sur le module UART lorsque 
* la tâche A et la tâche B étaient en cours. Maintenant, elle envoit cette information
* dans une queue.
*
* Parameters:
*  void * params : pointeur vers la structure tâche A ou B
*
* Return:
*  None
*
*
*******************************************************************************/
void print_loop(void * params)
{
    task_params_t * tache_en_cours = params;
    for (;;)
    {
        vTaskDelay(tache_en_cours->delay);
        //UART_PutString(tache_en_cours->message); (fonction du numéro 3)
        //On créé un buffer de la même forme que le message afin d'avoir le bon type
        char * buffer = tache_en_cours->message;
        //On envoit dans la queue l'adresse d'un pointeur de char (adresse d'une string)
        xQueueSend(print_queue, &buffer, 0);
    }
}
/*******************************************************************************
* Function Name: void print(void)
********************************************************************************
*
* Summary: Tâche qui affiche le message contenue dans la queue sur le module UART
* seulement lorsqu'il y a bien quelque chose dans la queue.
*
* Parameters:
*  None
*
* Return:
*  None
*
*
*******************************************************************************/
void print(void)
{
    //On initialise message comme un pointeur de char (string)
    char * message;
    for(;;)
    {
        //On reçoit de la queue l'adresse d'une string qu'on insère dans l'adresse de message
        if (xQueueReceive(print_queue, &message, portMAX_DELAY) == pdTRUE)
        {
            //Puisque message est un pointeur de char (string) on peut directement l'afficher
            UART_PutString(message);
        }
    }
}

int main(void)
{
    __enable_irq(); /* Enable global interrupts. */
    
    //On crée la tâche pour la LED avec la moins haute priorité
    xTaskCreate(LED_task, "LED", 500, NULL, 2, NULL);
    
    //On crée le sémaphore pour le bouton
    bouton_semph = xSemaphoreCreateBinary();
    
    //On crée la queue pour le print
    print_queue = xQueueCreate(2, sizeof(char *));
    
    UART_Start();
    
    Cy_SysInt_Init(&Bouton_ISR_cfg, isr_bouton);
    NVIC_ClearPendingIRQ(Bouton_ISR_cfg.intrSrc);
    NVIC_EnableIRQ(Bouton_ISR_cfg.intrSrc);
    
    //On crée les tâches pour le bouton, la tâche A, la tâche B et le print
    xTaskCreate(bouton_task, "BOUTON", 500, NULL, 2, NULL);
    xTaskCreate(print_loop, "task A", configMINIMAL_STACK_SIZE, (void *) &task_A, 1, NULL);
    xTaskCreate(print_loop, "task B", configMINIMAL_STACK_SIZE, (void *) &task_B, 1, NULL);
    xTaskCreate(print, "print", configMINIMAL_STACK_SIZE, NULL, 0, NULL);
    
    //On start le scheduler en fonction des priorités associées aux tâches
    vTaskStartScheduler();

    for(;;)
    {
        /* Place your application code here. */
    }
}

/* [] END OF FILE */
