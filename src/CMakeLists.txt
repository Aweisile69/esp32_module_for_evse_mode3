# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# idf_component_register(SRCS ${app_sources})
idf_component_register(SRCS "main.c"
                            "../user_lib/spiffs_api/api_spiffs.c"
                       INCLUDE_DIRS "../user_lib/spiffs_api/")

# 在构建系统中添加命令创建分区镜像
# storage           -分区名
# ../spiffs_image   -分区镜像文件路径
# FLASH_IN_PROJECT  -将分区镜像与应用二进制程序和分区表一起自动烧录到设备中
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)