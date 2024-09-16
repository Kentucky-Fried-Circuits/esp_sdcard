/**
 * 08/09/2023 - Ruizhe He, this file contains functions related to data logging.
 * startLogging -> Call to start the logging task
 * stopLogging  -> Call to stop the logging task
 * isLoggingOn  -> Call to check if the logging task is running
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

#define HEADER1 "Battery Internal Date (UTC),Voltage,Current,Temperature,Faults"
#define HEADER_SIZE 100

TaskHandle_t task_handle = NULL;
TaskHandle_t memory_task = NULL;
const TickType_t xDelay = 14500 / portTICK_PERIOD_MS;
const char *TAG_DATALOG = "Data_Logging";

/**
 * The basic task for logging live data into csv file in SD card. If the csv file doesn't exist, it will create one.
 *
 * This component is updated to be used in v5.1+ and allows the auto mounting of SD card.
 */
void dataNowLog(void *pv_args)
{
    std::string str;

    for (;;)
    {
        if (!isMounted())
        {
            start_sd_card_and_Logging();
            if (memory_task == NULL)
                xTaskCreate(memoryTask, "memoryTask", configMINIMAL_STACK_SIZE * 5, NULL, 4, &memory_task);
        }

        char fileName[FILE_NAME_SIZE];
        char headers[HEADER_SIZE];
        str.append(HEADER1);

        if (!hasFile(fileName))
        {
            if (logStringToFile(headers, fileName))
                ESP_LOGI(TAG_DATALOG, "Created a new file %s", fileName);
            else
                unmount_sd_card();
        }

        str.clear();
        str.append();

        if (!logStringToFile(str, fileName))
            unmount_sd_card();

        str.clear();

        /* This function is used for checking memory leak */
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
            ESP_LOGI("raydebug", "try to remove old file");
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
}

/**
 * Stop the logging of live data when the function is called.
 */
void stopLogging()
{
    if (memory_task != NULL)
    {
        vTaskDelete(memory_task);
        memory_task = NULL;
    }
}

/**
 * Check if the logging task is running. Return 1 if it is running
 * and return 0 if it is not.
 */
int isLoggingOn()
{
    if (memory_task && task_handle && isMounted())
        return 1;
    return 0;
}

void begin_SD()
{
    startLogging();
}