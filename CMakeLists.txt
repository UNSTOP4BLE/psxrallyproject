cmake_minimum_required(VERSION 3.25)

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
include(cmake/tools.cmake)

# PC-specific settings
if(TARGET_PLATFORM STREQUAL "PC")
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_C_STANDARD 11)        
    include(FetchContent)

    # GLFW (auto-download from GitHub)
    FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw)

    # OpenGL (system-provided)
    find_package(OpenGL REQUIRED)
else()
    include(cmake/setup.cmake)
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
        src/commonlib/ps1/pcdrv.s
        src/commonlib/vendor/printf.c
    )
    target_include_directories(
        common PUBLIC
        src/commonlib
        src/commonlib/libc
    )
    link_libraries(common)
endif()
# Grab everything under src recursively
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.*"
)

# Remove commonlib subfolder
list(FILTER SRC_FILES EXCLUDE REGEX ".*/commonlib/.*")
if(TARGET_PLATFORM STREQUAL "PC")
    list(FILTER SRC_FILES EXCLUDE REGEX ".*/psx/.*")
    add_executable(PSXRallyProject ${SRC_FILES})
else()
    list(FILTER SRC_FILES EXCLUDE REGEX ".*/pc/.*")
    addPS1Executable(PSXRallyProject ${SRC_FILES})
endif()

target_compile_definitions(PSXRallyProject PUBLIC PLATFORM_${TARGET_PLATFORM})

if(TARGET_PLATFORM STREQUAL "PC")
    target_include_directories(PSXRallyProject PRIVATE src/pc/include)
    target_link_libraries(PSXRallyProject PRIVATE glfw OpenGL::GL)
endif()

#build images
file(GLOB_RECURSE _images
    RELATIVE ${PROJECT_SOURCE_DIR}
    assets/*.png
)
list(FILTER _images EXCLUDE REGEX ".*/sourcefiles/.*")

foreach(_in ${_images})
    get_filename_component(_basename ${_in} NAME_WE)
    get_filename_component(_path ${_in} PATH)

    set(_out_dir ${_path})
    file(MAKE_DIRECTORY ${_out_dir})
    set(_out_xtex ${_out_dir}/${_basename}.xtex)
    set(_in_vram ${_path}/${_basename}.vram)

    convertImage(
        ${_in}      
        ${_out_xtex} 
        ${_in_vram}   
    )

    list(APPEND _built_images ${_out_xtex})
endforeach()

add_custom_target(Images ALL
    DEPENDS ${_built_images}
)

#build models
file(GLOB_RECURSE _models
    RELATIVE ${PROJECT_SOURCE_DIR}
    assets/*.obj
)
list(FILTER _models EXCLUDE REGEX ".*/sourcefiles/.*")

foreach(_in ${_models})
    get_filename_component(_basename ${_in} NAME_WE)
    get_filename_component(_path ${_in} PATH)

    set(_out_dir ${_path})
    file(MAKE_DIRECTORY ${_out_dir})
    set(_out_xmdl ${_out_dir}/${_basename}.xmdl)
    set(_tex_dir ${_path}/)

	convertModel(
        ${_in}      
        ${_out_xmdl} 
        ${_tex_dir}   
	)

    list(APPEND _built_models ${_out_xmdl})
endforeach()

add_custom_target(Models ALL
    DEPENDS ${_built_models}
)

#build fonts
file(GLOB_RECURSE _fonts
    RELATIVE ${PROJECT_SOURCE_DIR}
    assets/*.json
)
list(FILTER _fonts EXCLUDE REGEX ".*/sourcefiles/.*")

foreach(_in ${_fonts})
    get_filename_component(_basename ${_in} NAME_WE)
    get_filename_component(_path ${_in} PATH)

    set(_out_dir ${_path})
    file(MAKE_DIRECTORY ${_out_dir})
    set(_out_xfnt ${_out_dir}/${_basename}.xfnt)

	convertFont(
        ${_in}      
        ${_out_xfnt} 
	)

    list(APPEND _built_fonts ${_out_xfnt})
endforeach()

add_custom_target(Fonts ALL
    DEPENDS ${_built_fonts}
)


add_custom_target(Assets ALL)
#textures
add_dependencies(Assets Images)
#models
add_dependencies(Models Images) #model depends on images
add_dependencies(Assets Models)
#fonts
add_dependencies(Assets Fonts)

add_dependencies(PSXRallyProject Assets)
