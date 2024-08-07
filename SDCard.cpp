/**
 * 08/04/2023 - Ruizhe He, this source file contains code to mount and unmount
 * a SD card from the ESP 32 board. Comes with other supporting functions
 * that will be used for logging and file manipulating.
 *
 * If RMK board is using SPI / JTAG, SDMMC will not work. We can only use
 * external SPI sd card. However, since GPIO 12 is also used for
 * internal flash voltage control. We have to use espefuse.py set_flash_voltage 3.3V
 * to set the flash voltage to 3.3V manually.
 *
 * The SPI bus initialization is commented out because some other components will start the bus.
 * If your project only uses SD card with SPI bus, you should initilize the SPI bus here.
 * If there are multiple SPI devices, they can share other data line, but they need to
 * have individual cs line. Any GPIO can act as a CS port.
 *
 * Check out the SDCard.h define to see which protocol is used.
 */
#include "SDCard.h"

static sdmmc_card_t *card;

/**
 * This function will mount the SD card. Since the logging task has to be done
 * with the sd card inserted, we start the logging task after the sd card is mounted.
 */
esp_err_t start_sd_card_and_Logging(void)
{
    if (isMounted())
        unmount_sd_card();

    esp_err_t ret;

#ifdef SD_MMC
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, // Format the SD card if failed to mount
        .max_files = MAX_FILES,          // Max amount of files opening at one time
        .allocation_unit_size = 16 * 1024};

    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
#endif
#ifdef SD_SPI
    sdspi_device_config_t device_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    device_config.host_id = SPI2_HOST;
    device_config.gpio_cs = SD_CS_PORT;

    ESP_LOGI(TAG_SD, "Initializing SD card");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = device_config.host_id;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    // spi_bus_config_t bus_cfg = {
    //     .mosi_io_num = GPIO_NUM_13,
    //     .miso_io_num = GPIO_NUM_12,
    //     .sclk_io_num = GPIO_NUM_14,
    //     .quadwp_io_num = -1,
    //     .quadhd_io_num = -1,
    //     .max_transfer_sz = 4000,
    // };
    // ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG_SD, "Failed to initialize bus.");
    //     return ESP_OK;
    // }

    ESP_LOGI(TAG_SD, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &device_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG_SD, "Failed to mount filesystem. "
                             "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
            return ESP_FAIL;
        }
        else
        {
            ESP_LOGE(TAG_SD, "Failed to initialize the card (%s). "
                             "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
            return ESP_FAIL;
        }
        return ESP_FAIL;
    }
    ESP_LOGI(TAG_SD, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

#endif

    if (ret == ESP_OK)
    {
        startLogging();
    }
    return ret;
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
 * Who knows if this works.
 *
 * Go through the sd card and remove the oldest file.
 */
void removeOldestFile()
{
    // Open directory
    DIR *dir = opendir(MOUNT_POINT);
    if (!dir)
    {
        ESP_LOGE(TAG_SD, "Failed to open directory");
        return;
    }

    struct dirent *ent;
    struct stat st;
    time_t oldest_time = LONG_MAX;
    char oldest_file[FILE_NAME_SIZE];

    // Iterate over files in the directory
    while ((ent = readdir(dir)) != NULL)
    {
        char file_path[sizeof(ent->d_name) + sizeof(MOUNT_POINT)];
        snprintf(file_path, sizeof(file_path), "%s/%s", MOUNT_POINT, ent->d_name);
        // Get file info
        if (stat(file_path, &st) == 0)
        {
            // Check if the file is a regular file and find the oldest one
            if (S_ISREG(st.st_mode) && st.st_mtime < oldest_time)
            {
                oldest_time = st.st_mtime;
                snprintf(oldest_file, sizeof(oldest_file), file_path);
            }
        }
        vTaskDelay(1);
    }

    closedir(dir);

    // Remove the oldest file
    if (remove(oldest_file) != 0)
    {
        ESP_LOGE(TAG_SD, "Failed to remove the oldest file, %s", oldest_file);
    }
    else
    {
        ESP_LOGI(TAG_SD, "Removed oldest file: %s", oldest_file);
    }
}

/**
 * This function will write the parameter string into a csv file. If the file doesn't exist,
 * it will create a file in the sd card with the fileName. It's fopen "a", so new data will be appended to the end.
 */
int logStringToFile(const char *formattedString, char *fileName)
{
    if (card == NULL)
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
 * the SD card has it or not.
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
