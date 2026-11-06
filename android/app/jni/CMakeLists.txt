cmake_minimum_required(VERSION 3.6)
project(OUTFOXIES_ANDROID C)

# SDL2 se clona en jni/SDL durante la compilacion
add_subdirectory(SDL)

set(OFX_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
add_library(main SHARED
  ${OFX_ROOT}/src/app/main.c
  ${OFX_ROOT}/src/core/nb2.c
  ${OFX_ROOT}/third_party/miniz/miniz.c
  ${OFX_ROOT}/third_party/musashi/m68kcpu.c
  ${OFX_ROOT}/third_party/musashi/m68kops.c
  ${OFX_ROOT}/third_party/musashi/m68kdasm.c
  ${OFX_ROOT}/third_party/musashi/softfloat/softfloat.c)
target_include_directories(main PRIVATE
  ${OFX_ROOT}/src/core
  ${OFX_ROOT}/third_party/miniz
  ${OFX_ROOT}/third_party/musashi)
target_compile_options(main PRIVATE -O2 -w)
target_link_options(main PRIVATE "-Wl,-z,max-page-size=16384")
target_link_libraries(main SDL2 m)
