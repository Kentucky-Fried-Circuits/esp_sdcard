idf_component_register(SRCS "SDCard.cpp" "dataLogging.cpp"
REQUIRES "sdmmc" "fatfs" "vfs" "bmk"
                    INCLUDE_DIRS "include")
