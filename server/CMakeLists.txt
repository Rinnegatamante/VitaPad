cmake_minimum_required(VERSION 2.8)


if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()


project(VitaPad)
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(VITA_APP_NAME "VitaPad")
set(VITA_TITLEID  "VPAD00001")
set(VITA_VERSION  "01.03")

# Vita's IP to send over FTP
set(VITA_IP  "192.168.1.34")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)
include_directories(
  ../include
)
file(GLOB SRC_FLATBUFFERS 
  ../common/*.fbs
)

foreach(FILE ${SRC_FLATBUFFERS})
  get_filename_component(BASE_NAME ${FILE} NAME_WE)
  set(FL_NAME " ${CMAKE_SOURCE_DIR}/../include/${BASE_NAME}.fbs.hpp")
  list(APPEND FLATBUFFERS_OUT ${FL_NAME})
  add_custom_command(OUTPUT ${FL_NAME}
    COMMAND "flatc"
    ARGS --cpp --scoped-enums 
    ARGS --gen-mutable
    ARGS --filename-suffix ".fbs" 
    ARGS --filename-ext "hpp"
    ARGS -o  ${CMAKE_SOURCE_DIR}/../include/
    ARGS ${FILE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${FILE}
  )
endforeach()

add_executable(${VITA_APP_NAME}
  src/ctrl.cpp
  src/main.cpp
  ${FLATBUFFERS_OUT}
)

target_link_libraries(${VITA_APP_NAME}
  stdc++
  vita2d
  SceSysmodule_stub
  SceCtrl_stub
  SceTouch_stub
  m
  SceNet_stub
  SceNetCtl_stub
  SceAppUtil_stub
  ScePgf_stub
  jpeg
  freetype
  c
  ScePower_stub
  SceCommonDialog_stub
  png16
  z
  SceGxm_stub
  SceDisplay_stub
  SceMotion_stub
  debugnet
  pthread
)

vita_create_self(${VITA_APP_NAME}.self ${VITA_APP_NAME})
vita_create_vpk(${VITA_APP_NAME}.vpk ${VITA_TITLEID} ${VITA_APP_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)

# Send the VPK directly to the PS Vita

add_custom_target(ftp_send "unzip" -o ${VITA_APP_NAME}.vpk -d ${VITA_APP_NAME}-extracted
  COMMAND "curl" --ftp-method nocwd -T ${VITA_APP_NAME}-extracted/eboot.bin "ftp://${VITA_IP}:1337/ux0:/app/${VITA_TITLEID}/"
  COMMAND "echo" destroy | nc ${VITA_IP} 1338
  COMMAND "echo" launch ${VITA_TITLEID} | nc ${VITA_IP} 1338
  DEPENDS ${VITA_APP_NAME}.vpk
)