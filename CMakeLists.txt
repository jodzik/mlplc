cmake_minimum_required(VERSION 3.31)

# Keep before find zephyr package!
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(mlplc)

message(DTS_ROOT=${DTS_ROOT})
message(BOARD_ROOT=${BOARD_ROOT})

aux_source_directory(src MLPLC_SRC)
target_sources(app PRIVATE ${MLPLC_SRC})
target_include_directories(app PUBLIC include)
target_include_directories(app PUBLIC src)
target_include_directories(app PUBLIC magic_enum/include)
