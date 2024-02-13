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

#define MAX_FILES 3
#define MOUNT_POINT "/sdcard"
const char *const TAG_SD = "SD_Card";

esp_err_t initi_sd_card_and_Logging(void);
esp_err_t unmount_sd_card(void);
int logStringToFile(const char *formattedString, char *fileName);
bool isMounted(void);
bool SD_getFreeSpace(uint32_t *tot, uint32_t *free);
void deleteFile(char *filePath);
int hasFile(char *fileName);
void memoryLogging(char *time_str);

// dataLogging.cpp
void startLogging();
void stopLogging();

#endif // SDCARD_H