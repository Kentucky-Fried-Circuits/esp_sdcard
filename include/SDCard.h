#pragma once

#ifndef SDCARD_H
#define SDCARD_H

#include <stdio.h>
#include <string>
#include <errno.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "dirent.h"

// #define SD_MAGNETHEREAL
#define SD_LESS

#ifdef SD_MAGNETHEREAL
#define SD_MMC
#endif
#ifdef SD_LESS
#define SD_SPI
#endif

#ifdef SD_LESS
#include "smartBattery.h"
#define SD_CS_PORT GPIO_NUM_32
#endif

#define MAX_FILES 3
#define MOUNT_POINT "/sdcard"
#define FILE_NAME_SIZE 100
const char *const TAG_SD = "SD_Card";

esp_err_t start_sd_card_and_Logging(void);
esp_err_t unmount_sd_card(void);
int logStringToFile(const char *formattedString, char *fileName);
bool isMounted(void);
bool SD_getFreeSpace(uint32_t *tot, uint32_t *free);
void deleteFile(char *filePath);
int hasFile(char *fileName);
void memoryLogging(char *time_str);
void removeOldestFile(void);

// dataLogging.cpp
void startLogging();
void stopLogging();
void SDCard_Task(void* arg);

#endif // SDCARD_H