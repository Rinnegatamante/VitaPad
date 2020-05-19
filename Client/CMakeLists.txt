cmake_minimum_required(VERSION 2.8)
# VitaPad
set(PROJECT VitaPad)

project(${PROJECT})

file(GLOB SRC_FLATBUFFERS 
  ../common/*.fbs
)

foreach(FILE ${SRC_FLATBUFFERS})
  get_filename_component(BASE_NAME ${FILE} NAME_WE)
  set(FL_NAME "../include/${BASE_NAME}.fbs.hpp")
  list(APPEND FLATBUFFERS_OUT ${FL_NAME})
  add_custom_command(OUTPUT ${FL_NAME}
    COMMAND "flatc"
    ARGS --cpp --scoped-enums 
    ARGS --gen-mutable
    ARGS --filename-suffix ".fbs" 
    ARGS --filename-ext "hpp"
    ARGS -o ../include
    ARGS ${FILE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${FILE}
  )
endforeach()
include_directories(
  ../include
)

if (UNIX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

add_executable(${PROJECT}
    main.cpp
    ctrl.cpp
    sock_data.cpp
    uinput.c
    ${FLATBUFFERS_OUT}
)
include_directories(
  /usr/include/libevdev-1.0/
)
target_link_libraries(${PROJECT}
    evdev
)

elseif (WIN32)

add_custom_command(OUTPUT  vendor/lib/release/x64/ViGEmClient.lib
                   COMMAND vendor/build.ps1 -configuration Release_LIB|x64
                   WORKING_DIRECTORY vendor/)
add_executable(${PROJECT}
    main.cpp
    vigem.c
    ${FLATBUFFERS_OUT}
)
target_link_libraries(${PROJECT} vendor/lib/release/x64/ViGEmClient.lib)

endif (UNIX)