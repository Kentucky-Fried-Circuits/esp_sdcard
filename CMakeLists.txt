idf_component_register(SRCS "SDCard.cpp" "dataLogging.cpp"
REQUIRES "sdmmc" "fatfs" "vfs"
                    INCLUDE_DIRS "include")
