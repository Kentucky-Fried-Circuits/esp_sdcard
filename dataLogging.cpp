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
 */

#ifdef SD_LESS
#include "smartBattery.h"
#endif

#include "SDCard.h"

#define HEADER1 "Battery Internal Date (UTC),Voltage,Current,Temperature,Faults"
#define HEADER_SIZE 100

TaskHandle_t task_handle = NULL;
TaskHandle_t memory_task = NULL;
const TickType_t xDelay = 14500 / portTICK_PERIOD_MS;
const char *TAG_DATALOG = "Data_Logging";
extern smartBattery smartBattery;

/**
 * The basic task for logging live data into csv file in SD card. This function
 * will run roughly every 15 seconds. If the csv file doesn't exist, it will create one.
 */
void dataNowLog(void *pv_args)
{
    std::vector<std::string> vec;
    std::string str;
    for (;;)
    {
        char fileName[FILE_NAME_SIZE];
        char buffer[150];
        char headers[HEADER_SIZE];

#ifdef SD_LESS
        bat_time bt = smartBattery.get_battery_time();
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

#endif
        logStringToFile(buffer, fileName);

        /* This function is used for checking memory leak */
        // memoryLogging(time_str);

        vTaskDelay(xDelay);
    }
}

/**
 * If we have less than 1000kb in the sd card, remove the oldest file in the sdcard.
 */
void memoryTask(void *param)
{
    uint32_t total;
    uint32_t free;

    while (1)
    {
        SD_getFreeSpace(&total, &free);
        if (free < 1000)
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
    xTaskCreate([](void *param)
                {
        while(1){
            struct timeval tv_now;
            gettimeofday(&tv_now, NULL);
            char timeString[26];  // The length of the string is fixed for ctime
            ctime_r(&tv_now.tv_sec, timeString);
            ESP_LOGI("raydebug","logging memory %s", timeString);
            memoryLogging(timeString);

            vTaskDelay(500);} },
                "memoryLog", 4000, NULL, 5, NULL);

    // if (task_handle == NULL)
    //     xTaskCreate(dataNowLog, "dataLoggingTask", configMINIMAL_STACK_SIZE * 4, NULL, 10, &task_handle);

    if (memory_task == NULL)
        xTaskCreate(memoryTask, "memoryTask", configMINIMAL_STACK_SIZE * 4, NULL, 5, &memory_task);
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