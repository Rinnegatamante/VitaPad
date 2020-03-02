cmake_minimum_required(VERSION 2.8)
# VitaPad
set(PROJECT VitaPad)

project(${PROJECT})

if (UNIX)

add_executable(${PROJECT}
    main.c    
    uinput.c
)
include_directories(/usr/include/libevdev-1.0/ ${CMAKE_SOURCE_DIR}/../include)
target_link_libraries(${PROJECT}
    evdev
)

elseif (WIN32)

add_custom_command(OUTPUT  vendor/lib/release/x64/ViGEmClient.lib
                   COMMAND vendor/build.ps1 -configuration Release_LIB|x64
                   WORKING_DIRECTORY vendor/)
add_custom_target(
    VigemLib ALL
    DEPENDS vendor/lib/release/x64/ViGEmClient.lib
)
add_executable(${PROJECT}
    main.c
    vigem.c
)
target_link_libraries(${PROJECT} ${CMAKE_SOURCE_DIR}/vendor/lib/release/x64/ViGEmClient.lib)

endif (UNIX)