cmake_minimum_required(VERSION 3.10)

project(Ism VERSION 0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include_directories(
    include
    src
)
configure_file(src/ism.h.in include/ism.h)

add_library(fastrng STATIC src/cccompat.h src/fastrng.h src/fastrng.c)
add_library(perlin STATIC src/perlin.h src/perlin.c)
add_library(map STATIC src/map.h src/map.c)
add_library(view STATIC src/mainview.h src/mainview.c)

set(RESOURCE_FILES
    resources/Hack-Regular.ttf
    resources/Potash-8x8.png
    resources/Potash-10x10.png
    resources/tiles-10x10.png
    resources/mountain-30x30.png
    resources/mountain-20x20.png
    resources/mountain-30x20.png
)

add_executable(ism
    src/ism.c
    ${RESOURCE_FILES}
)

IF(APPLE)
    # Build app bundle
    set(BEARLIBTERMINAL_LIB "${PROJECT_SOURCE_DIR}/lib/libBearLibTerminal.dylib")
    # TODO icon and plist
    set_target_properties(ism PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_FRAMEWORK_IDENTIFIER org.caseman.ism
        RESOURCE "${RESOURCE_FILES}"
    )
    set(BUNDLE_FRAMEWORK_PATH "${CMAKE_BINARY_DIR}/ism.app/Contents/MacOS")
    file(COPY ${BEARLIBTERMINAL_LIB} DESTINATION ${BUNDLE_FRAMEWORK_PATH}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE
    )
    # target_link_libraries(ism "-framework Foundation" "-framework Cocoa" objc)
ENDIF(APPLE)
# TODO Linux, etc.

target_link_libraries(ism fastrng perlin map view
    ${BEARLIBTERMINAL_LIB}
)
target_include_directories(ism PUBLIC
    "${PROJECT_BINARY_DIR}/include"
)

add_executable(genmap src/genmap.c)
target_link_libraries(genmap fastrng perlin map)
target_include_directories(genmap PUBLIC
    "${PROJECT_BINARY_DIR}/include"
)

add_executable(testview src/testview.c)
target_link_libraries(testview ${BEARLIBTERMINAL_LIB})
file(COPY ${BEARLIBTERMINAL_LIB} DESTINATION ${CMAKE_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE
)
target_include_directories(testview PUBLIC
    "${PROJECT_BINARY_DIR}/include"
)
