cmake_minimum_required(VERSION 3.17)

project(videotools CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FILE(GLOB sources *.c *.h *.cpp)

# we only care about building on linux
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(${PROJECT_NAME}
    PkgConfig::LIBAV
)

# turn off the shit about c++ not supporting c99 features, the only compiler where this is a problem is MSVC and we're not supporting windows
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-c99-designator -Wno-reorder-init-list)

# trust that the ffmpeg devs knew what they were doing comparing an enum value to -1
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-tautological-constant-out-of-range-compare)