/**
 * 08/04/2023 - Ruizhe He, this source file contains code to mount and unmount
 * a SD card from the ESP 32 board. Comes with other supporting functions
 * that will be used for logging and file manipulating.
 */
#include "SDCard.h"

static sdmmc_card_t *card;

/**
 * This function will mount the SD card. Since the logging task has to be done
 * with the sd card inserted, we start the logging task after the sd card is mounted.
 */
esp_err_t initi_sd_card_and_Logging(void)
{
    if (isMounted())
        unmount_sd_card();

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, // Format the SD card if failed to mount
        .max_files = MAX_FILES           // Max amount of files opening at one time
    };

    esp_err_t err = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG_SD, "Mounted");
        startLogging();
    }
    return err;
}

/**
 * This function will unmount the sd card. Since logging task requires SD card to
 * be mounted. This function will also stop the logging task to avoid error.
 */
esp_err_t unmount_sd_card(void)
{
    esp_err_t err = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);

    if (err == ESP_OK)
    {
        stopLogging();
        card = NULL;
        ESP_LOGI(TAG_SD, "SD card is unmounted. Logging task ended.");
    }

    return err;
}

/**
 * This function will write the parameter string into a csv file. If the file doesn't exist,
 * it will create a file in the sd card with the fileName. It's fopen "a", so new data will be appended to the end.
 */
int logStringToFile(const char *formattedString, char *fileName)
{
    if (card != NULL)
        return 0;

    std::string fullPath;
    fullPath.append(MOUNT_POINT).append("/").append(fileName);

    FILE *f = fopen(fullPath.c_str(), "a");
    if (f == NULL)
    {
        ESP_LOGD(TAG_SD, "Failed to open file for writing for %s", fullPath.c_str());
        return 0;
    }

    fprintf(f, "%s\n", formattedString);
    fclose(f);

    return 1;
}

/**
 * Retrun a bool value whether the SD card is mounted or not
 */
bool isMounted(void)
{
    return card != NULL;
}

/**
 * This function will return a bool value whether we retrieved the value
 * successfully or not. Two parameters will be used to pass in and out
 * the free space and total space. If we can't read the sd card,
 * we need to unmount the sd card and stop the logging task.
 *
 * Value is in KB format.
 */
bool SD_getFreeSpace(uint32_t *total, uint32_t *free)
{
    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;
    FRESULT result = f_getfree("0:", &fre_clust, &fs);
    /* Get volume information and free clusters of drive 0 */
    if (result == FR_OK)
    {
        /* Get total sectors and free sectors */
        tot_sect = (fs->n_fatent - 2) * fs->csize;
        fre_sect = fre_clust * fs->csize;

        *total = tot_sect / 2;
        *free = fre_sect / 2;

        return true;
    }
    else if (result == FR_NOT_READY)
    {
        unmount_sd_card();
    }
    return false;
}

/*Delete specific file within the sd card*/
void deleteFile(char *filePath)
{
    unlink(filePath);
}

/**
 * Check if a file exists in the SD card or not. Return 1 or 0 depends on whether
 * the SD card has it or not. If there is no sd card, we unmount the sd card.
 */
int hasFile(char *fileName)
{
    struct stat st;
    std::string filePath;
    filePath.append(fileName);

    if (filePath.find(MOUNT_POINT) == std::string::npos)
    {
        std::string temp;
        temp.append(MOUNT_POINT).append("/");
        filePath.insert(0, temp);
    }
    if (stat(filePath.c_str(), &st) == 0)
        return 1;

    return 0;
}

/**
 * This is used for memory leak debugging
 */
void memoryLogging(char *time_str)
{
    std::string fullPath;
    fullPath.append(MOUNT_POINT).append("/").append("logging.csv");
    FILE *f = fopen(fullPath.c_str(), "a");
    if (f == NULL)
    {
        ESP_LOGE(TAG_SD, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Current time is %s, %zu\n", time_str, esp_get_free_heap_size());
    fclose(f);
}
