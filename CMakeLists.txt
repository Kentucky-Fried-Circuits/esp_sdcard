idf_component_register(SRCS "SDCard.cpp" "dataLogging.cpp"
REQUIRES "sdmmc" "fatfs" "vfs" "blueSkyModbus" "ds3231" "i2cdev"
                    INCLUDE_DIRS "include")
