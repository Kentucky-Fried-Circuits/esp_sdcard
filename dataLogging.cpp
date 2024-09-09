/**
 * 08/29/2024 - Ruizhe He, this file contains functions related to data logging. Updated for hypr 3K logging.
 * startLogging -> Call to start the logging task
 * stopLogging  -> Call to stop the logging task
 * SD card is mounting function is called here.
 *
 * This file is used to log data into SD card, either the internal ones or the external
 * SPI SD Card. Depends on what define we are using, we can choose between what logging
 *  we are doing.
 *
 * For the purpose of logging data into the SD card, use dataNowLog, modify it and the header
 * to what you need.
 */
#include "SDCard.h"

#define BUFFER_SIZE 300

TaskHandle_t task_handle = NULL;
TaskHandle_t memory_task = NULL;
const TickType_t xDelay = 5000 / portTICK_PERIOD_MS;
const char *TAG_DATALOG = "Data_Logging";

/**
 * The basic task for logging live data into csv file in SD card. This function
 * will run roughly every 6 seconds. If the csv file doesn't exist, it will create one.
 *
 * A header will be dynamically created from another class since we may expand what we will log.
 * Logging value will be retrieved as string with correct format and unit.
 */
void dataNowLog(void *pv_args)
{
    std::string str;

    vTaskDelay(5000);

    for (;;)
    {

        // memoryLogging(time_str);

        vTaskDelay(xDelay);
    }
}

/**
 * If we have less than 10000kb in the sd card, remove the oldest file in the sdcard.
 */
void memoryTask(void *param)
{
    uint32_t total;
    uint32_t free;

    while (1)
    {
        SD_getFreeSpace(&total, &free);
        if (free < 10000)
        {
            ESP_LOGI(TAG_DATALOG, "try to remove old file");
            removeOldestFile();
        }
        vTaskDelay(xDelay * 10);
    }
}

/**
 * Since logging task requires SD card to be mounted, this function
 * will be called by the init_sd_card function. Users can choose to
 * mount or unmount the sd card through the UI.
 */
void startLogging()
{
    if (task_handle == NULL)
        xTaskCreate(dataNowLog, "dataLoggingTask", configMINIMAL_STACK_SIZE * 5, NULL, 6, &task_handle);

    if (memory_task == NULL)
        xTaskCreate(memoryTask, "memoryTask", configMINIMAL_STACK_SIZE * 5, NULL, 4, &memory_task);
}

/**
 * Stop the logging of live data when the function is called.
 */
void stopLogging()
{
    if (task_handle != NULL)
    {
        vTaskDelete(task_handle);
        task_handle = NULL;
    }
    if (memory_task != NULL)
    {
        vTaskDelete(memory_task);
        memory_task = NULL;
    }
}

/*Allow us to automatically mount and unmount SD card
as long as this task is running*/
void SDCard_Task(void *arg)
{
    // /*Wait for the bus to come online before starting the sd card*/
    vTaskDelay(1500);
    uint32_t tot = 0;
    uint32_t free = 0;

    while (1)
    {

        if (!isMounted())
        {
            start_sd_card_and_Logging();
        }

        SD_getFreeSpace(&tot, &free);

        vTaskDelay(2000);
    }
}

void begin_SD()
{
    xTaskCreate(SDCard_Task, "SDcard_task", configMINIMAL_STACK_SIZE * 5, NULL, 7, NULL);
}