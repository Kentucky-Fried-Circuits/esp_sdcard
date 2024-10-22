idf_component_register(SRCS "SDCard.cpp" "dataLogging.cpp"
REQUIRES "sdmmc" "fatfs" "vfs" "bmk" "LESS_AGS"
                    INCLUDE_DIRS "include")
