# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)

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
    ../cocos2d-x/cocos)

#set_target_properties(spineruntime
#                      PROPERTIES
#                      ARCHIVE_OUTPUT_DIRECTORY
#                      "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/android/${CMAKE_BUILD_TYPE}/${ANDROID_ABI}")

#set (LIB_BUILD_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../../../../../../.externalNativeBuild/cmake/${CMAKE_BUILD_TYPE}/${ANDROID_ABI})

# Create target which consume the command via DEPENDS.
set (LIB_PUBLISH_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../../../../../../../prebuilt_libs/android/${CMAKE_BUILD_TYPE}/${ANDROID_ABI})
file(MAKE_DIRECTORY ${LIB_PUBLISH_DIRECTORY})
add_custom_command(TARGET spineruntime POST_BUILD
                   COMMAND "${CMAKE_COMMAND}" -E
                   copy "${CMAKE_BINARY_DIR}/libspineruntime.a"
                   "${LIB_PUBLISH_DIRECTORY}/libspineruntime.a"
                   COMMENT "Copying library to output directory")



