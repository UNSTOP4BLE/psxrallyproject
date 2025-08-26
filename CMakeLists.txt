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
	assets/cascube/cas1.png 4
	assets/cascube/cas1.dat
	assets/cascube/cas1pallete.dat
)
convertImage(
	assets/cascube/cas2.png 4
	assets/cascube/cas2.dat
	assets/cascube/cas2pallete.dat
)
convertImage(
	assets/cascube/cas3.png 4
	assets/cascube/cas3.dat
	assets/cascube/cas3pallete.dat
)
convertImage(
	assets/cascube/cas4.png 4
	assets/cascube/cas4.dat
	assets/cascube/cas4pallete.dat
)
convertImage(
	assets/cascube/cas5.png 4
	assets/cascube/cas5.dat
	assets/cascube/cas5pallete.dat
)
convertImage(
	assets/cascube/cas6.png 4
	assets/cascube/cas6.dat
	assets/cascube/cas6pallete.dat
)

add_custom_target(converted_images ALL DEPENDS 
    assets/cascube/cas1.dat
    assets/cascube/cas2.dat
    assets/cascube/cas3.dat
    assets/cascube/cas4.dat
    assets/cascube/cas5.dat
    assets/cascube/cas6.dat)

add_dependencies(PSXRallyProject converted_images)

convertModel(
	assets/cascube/cascube.obj #in 
	assets/cascube/cascube.mdl #out 
	assets/cascube #texture directory
)

addBinaryFile(PSXRallyProject mymodel "${PROJECT_BINARY_DIR}/assets/cascube/cascube.mdl")#"${PROJECT_BINARY_DIR}/assets/textureData.dat")