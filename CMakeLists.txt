cmake_minimum_required(VERSION 3.16)
project(Outfoxies C)
set(CMAKE_C_STANDARD 99)

set(OFX_CORE
  src/core/nb2.c
  third_party/miniz/miniz.c
  third_party/musashi/m68kcpu.c
  third_party/musashi/m68kops.c
  third_party/musashi/m68kdasm.c
  third_party/musashi/softfloat/softfloat.c)
set(OFX_INCLUDES src/core third_party/miniz third_party/musashi)
add_compile_options(-O2 -w)

add_executable(headless tools/headless.c ${OFX_CORE})
target_include_directories(headless PRIVATE ${OFX_INCLUDES})
target_link_libraries(headless PRIVATE m)

find_package(SDL2 CONFIG)
if(SDL2_FOUND)
  add_executable(outfoxies src/app/main.c ${OFX_CORE})
  target_include_directories(outfoxies PRIVATE ${OFX_INCLUDES})
  if(TARGET SDL2::SDL2main)
    target_link_libraries(outfoxies PRIVATE SDL2::SDL2main)
  endif()
  if(TARGET SDL2::SDL2-static)
    target_link_libraries(outfoxies PRIVATE SDL2::SDL2-static m)
  else()
    target_link_libraries(outfoxies PRIVATE SDL2::SDL2 m)
  endif()
endif()
