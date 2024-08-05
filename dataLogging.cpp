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
#ifdef SD_LESS
extern smartBattery smartBattery;
#endif

/**
 * The basic task for logging live data into csv file in SD card. This function
 * will run roughly every 15 seconds. If the csv file doesn't exist, it will create one.
 */
void dataNowLog(void *pv_args)
{
    std::vector<std::string> vec;
    std::string str;
    bat_time bt;
    for (;;)
    {
        char fileName[FILE_NAME_SIZE];
        char buffer[150];
        char headers[HEADER_SIZE];

#ifdef SD_LESS

        bt = smartBattery.get_battery_time();
        snprintf(fileName, sizeof(fileName), "%d%d%d.csv", bt.month, bt.day, bt.year);
        if (!hasFile(fileName))
        {
            snprintf(headers, HEADER_SIZE, "%s", HEADER1);
            if (logStringToFile(headers, fileName))
                ESP_LOGI(TAG_DATALOG, "Created a new file %s", fileName);
        }

        str.clear();
        vec = smartBattery.get_err_msg();
        if (vec.empty())
            str.append("No error.");
        else
        {
            for (auto it : vec)
            {
                str.append(it).append(" ");
            }
        }

        // Battery Internal Date (UTC),Voltage,Current,Temperature,Faults
        snprintf(buffer, sizeof(buffer), "%d:%d,%0.2f, %0.2f,%d,%s",
                 bt.hours, bt.minutes, smartBattery.get_battery_voltage(),
                 smartBattery.get_battery_current(),
                 smartBattery.get_battery_internal_temp_c(),
                 str.c_str());
        logStringToFile(buffer, fileName);
#endif
        // char time_str[25];
        // snprintf(time_str, sizeof(time_str), "%d:%d:%d:%d:%d", bt.month, bt.day, bt.year, bt.hours, bt.minutes);
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