#pragma once

#ifndef SDCARD_H
#define SDCARD_H

#include <stdio.h>
#include <string>
#include <errno.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "esp_log.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "dirent.h"

#define MAX_FILES 3
#define MOUNT_POINT "/sdcard"
#define FILE_NAME_SIZE 100
const char *const TAG_SD = "SD_Card";

esp_err_t start_sd_card_and_Logging(void);
esp_err_t unmount_sd_card(void);
int logStringToFile(std::string formattedString, char *fileName);
bool isMounted(void);
bool SD_getFreeSpace(uint32_t *tot, uint32_t *free);
void deleteFile(char *filePath);
int hasFile(char *fileName);
void memoryLogging(char *time_str);
void removeOldestFile(void);

// dataLogging.cpp
void startLogging();
void stopLogging();
void begin_SD();

#endif // SDCARD_H