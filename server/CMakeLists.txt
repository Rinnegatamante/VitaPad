cmake_minimum_required(VERSION 3.0)

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
if(DEFINED ENV{VITA_IP})
  set(VITA_IP $ENV{VITA_IP})
endif()

if(DEFINED ENV{DEBUG_IP})
  add_compile_definitions(DEBUG_IP="$ENV{DEBUG_IP}")
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/")
include(BuildFlatBuffers)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)
include_directories(
  ../include
)
file(GLOB SRC_FLATBUFFERS
  ../common/*.fbs
)

build_flatbuffers("${SRC_FLATBUFFERS}"
 ""
 flatbuffers_target
 ${CMAKE_CURRENT_LIST_DIR}
 ${CMAKE_BINARY_DIR}/generated
 ""
 "")

include_directories(${CMAKE_BINARY_DIR}/generated)

add_executable(${VITA_APP_NAME}
  src/ctrl.cpp
  src/main.cpp
  src/net.cpp
)

target_compile_options(${VITA_APP_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_dependencies(${VITA_APP_NAME} flatbuffers_target)

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
  # SceDriverUser_stub
  SceDisplay_stub
  SceMotion_stub
  SceAppMgr_stub
  #
  ScePgf_stub
  jpeg
  freetype
  c
  ScePower_stub
  SceCommonDialog_stub
  png16
  z
  SceGxm_stub
  debugnet
  flatbuffers
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

if (VITA_IP)
  add_custom_target(ftp_send
    "unzip" -o ${VITA_APP_NAME}.vpk -d ${VITA_APP_NAME}-extracted
    COMMAND "curl" --ftp-method nocwd -T ${VITA_APP_NAME}-extracted/eboot.bin "ftp://${VITA_IP}:1337/ux0:/app/${VITA_TITLEID}/"
    # COMMAND "echo" destroy | nc ${VITA_IP} 1338
    # COMMAND "echo" launch ${VITA_TITLEID} | nc ${VITA_IP} 1338
    DEPENDS ${VITA_APP_NAME}.vpk
  )
endif()
