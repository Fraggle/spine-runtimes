# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.
cmake_minimum_required(VERSION 3.4.1)

project(spine)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.

file(GLOB_RECURSE SRC_FILES spine-cocos2dx/src/spine/*.cpp spine-c/spine-c/src/spine/*.c)

add_library( spineruntime STATIC ${SRC_FILES} )

target_include_directories(spineruntime PUBLIC
    spine-cocos2dx/src
    spine-c/spine-c/include
    spine-c/spine-c/src
    spine-c/spine-c/src/spine
    ../cocos2d-x/cocos
    ../cocos2d-x/external)

if(ANDROID)

    # Create target which consume the command via DEPENDS.
    get_filename_component(LIB_PUBLISH_DIRECTORY
                           "{CMAKE_SOURCE_DIR}/../../../build/built_libs/android/${CMAKE_BUILD_TYPE}/${ANDROID_ABI}"
                           ABSOLUTE)
    file(MAKE_DIRECTORY ${LIB_PUBLISH_DIRECTORY})

    add_custom_command(TARGET spineruntime POST_BUILD
                       COMMAND "${CMAKE_COMMAND}" -E
                       copy "${CMAKE_BINARY_DIR}/libspineruntime.a"
                       "${LIB_PUBLISH_DIRECTORY}/libspineruntime.a"
                       COMMENT "Copying spineruntime to output directory")
endif()

if(EMSCRIPTEN)

	# str(n)dup() is a POSIX 2008 extension, yet was natively added to BSD before it made it to posix.
	# It is therefore available on those platforms, but not on emscripten, unless _POSIX_SOURCE is defined
	# and set to a positive integer

	add_definitions(-D_POSIX_SOURCE=1)

endif()
