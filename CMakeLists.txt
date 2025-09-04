cmake_minimum_required(VERSION 3.25)

# Set the path to the toolchain file, which will configure CMake to use the MIPS
# toolchain rather than its default compiler and proceed in turn to execute
# setup.cmake.
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain.cmake")

# Tell CMake about the project. The VERSION, DESCRIPTION and HOMEPAGE_URL fields
# are optional, but the project name and LANGUAGES field should be present.
project(
	PSXRallyProject
	LANGUAGES    C CXX ASM
	VERSION      1.0.0
	DESCRIPTION  "PSXRallyProject"
	HOMEPAGE_URL "https://github.com/spicyjpeg/ps1-bare-metal"
)

# Set up compiler flags and initialize the Python environment used to run the
# scripts in the tools directory.
include(cmake/setup.cmake)
include(cmake/tools.cmake)

# Build a "common" library and link it
add_library(
	common OBJECT
	src/commonlib/libc/clz.s
	src/commonlib/libc/crt0.c
	src/commonlib/libc/cxxsupport.cpp
	src/commonlib/libc/malloc.c
	src/commonlib/libc/misc.c
	src/commonlib/libc/setjmp.s
	src/commonlib/libc/string.c
	src/commonlib/libc/string.s
	src/commonlib/ps1/cache.s
	src/commonlib/vendor/printf.c
)
target_include_directories(
	common PUBLIC
	src/commonlib
	src/commonlib/libc
)
link_libraries(common)

# Grab everything under src recursively
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.*"
)

# Remove commonlib subfolder
list(FILTER SRC_FILES EXCLUDE REGEX ".*/commonlib/.*")

addPS1Executable(PSXRallyProject ${SRC_FILES})

convertImage(
    assets/font.png
    assets/font.xtex
    assets/font.vram
)
addBinaryFile(PSXRallyProject g_myfonttex "${PROJECT_BINARY_DIR}/assets/font.xtex")#"${PROJECT_BINARY_DIR}/assets/textureData.dat")

convertImage(
    assets/texture.png
    assets/texture.xtex
    assets/texture.vram
)
addBinaryFile(PSXRallyProject g_mytex "${PROJECT_BINARY_DIR}/assets/texture.xtex")#"${PROJECT_BINARY_DIR}/assets/textureData.dat")

convertModel(
	assets/cube.obj #in 
	assets/cube.xmdl #out 
	assets/ #texture directory
)
addBinaryFile(PSXRallyProject g_mymodel "${PROJECT_BINARY_DIR}/assets/cube.xmdl")#"${PROJECT_BINARY_DIR}/assets/textureData.dat")

convertFont(
	assets/font.json #in 
	assets/font.xfnt #out 
)
addBinaryFile(PSXRallyProject g_myfontmap "${PROJECT_BINARY_DIR}/assets/font.xfnt")#"${PROJECT_BINARY_DIR}/assets/textureData.dat")