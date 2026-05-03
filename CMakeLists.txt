cmake_minimum_required(VERSION 3.10)
project(mank)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED libzip)

file(GLOB SRCS "src/*.cpp")

add_executable(mank ${SRCS})

target_link_libraries(mank PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    ${LIBZIP_LIBRARIES}
    yaml-cpp
)

target_include_directories(mank PRIVATE
    include/
    ${LIBZIP_INCLUDE_DIRS}
)


target_compile_options(mank PRIVATE -Wall -Wextra)