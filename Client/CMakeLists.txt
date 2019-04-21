# VitaPad
set(PROJECT VitaPad)

project(${PROJECT})

if (LINUX)
add_executable(${PROJECT}
    main.c    
    uinput.c
)
endif (LINUX) 

if (WIN32)
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
endif (WIN32)