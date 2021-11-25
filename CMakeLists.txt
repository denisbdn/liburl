cmake_minimum_required(VERSION 2.6)
project(Liburl C)

message(STATUS "Making ${PROJECT_NAME} on ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")

# Directories
set(LIBURL_SRC_DIR ".")

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CMakeParseArguments)

macro(AddCompilerFlags)
    CMAKE_PARSE_ARGUMENTS(
        COMPILER_FLAGS # Prefix
        "FAIL_ON_ERROR" # Options
        "BUILD_TYPE" # One value arguments
        "FLAGS;LANGUAGES" # Multi value arguments
        ${ARGN}
    )

    if (NOT "${COMPILER_FLAGS_BUILD_TYPE}" STREQUAL "")
        set(COMPILER_FLAGS_BUILD_TYPE "_${COMPILER_FLAGS_BUILD_TYPE}")
    endif()

    foreach(FLAG ${COMPILER_FLAGS_FLAGS})
        string(REGEX REPLACE "[+/:= -]" "_" FLAG_ESC "${FLAG}")

        foreach(LANGUAGE ${COMPILER_FLAGS_LANGUAGES})
            set(FLAG_ESC "${LANGUAGE}_${FLAG_ESC}")
            # Check language
            if("${LANGUAGE}" STREQUAL "C")
                check_c_compiler_flag(${FLAG} ${FLAG_ESC})
            elseif("${LANGUAGE}" STREQUAL "CXX")
                check_cxx_compiler_flag(${FLAG} ${FLAG_ESC})
            else()
                message(FATAL_ERROR "Language ${LANGUAGE} not supported")
            endif()

            # Check return status
            if(${FLAG_ESC})
                if("${LANGUAGE}" STREQUAL "C")
                    set(CMAKE_C_FLAGS${COMPILER_FLAGS_BUILD_TYPE} "${CMAKE_C_FLAGS${COMPILER_FLAGS_BUILD_TYPE}} ${FLAG}")
                elseif("${LANGUAGE}" STREQUAL "CXX")
                    set(CMAKE_CXX_FLAGS${COMPILER_FLAGS_BUILD_TYPE} "${CMAKE_CXX_FLAGS${COMPILER_FLAGS_BUILD_TYPE}} ${FLAG}")
                endif()
            elseif(${COMPILER_FLAGS_FAIL_ON_ERROR})
                message(FATAL_ERROR "${FLAG} not supported for ${LANGUAGE}. Try to update compiler/linker. Or don't set FAIL_ON_ERROR")
            endif()
        endforeach(LANGUAGE ${COMPILER_FLAGS_LANGUAGES})
    endforeach(FLAG ${COMPILER_FLAGS_LANGUAGES})
endmacro(AddCompilerFlags)

# AddCompilerFlags(FAIL_ON_ERROR FLAGS -Werror -Wextra -std=gnu99 LANGUAGES C)
AddCompilerFlags(FAIL_ON_ERROR FLAGS -Wextra -std=gnu99 LANGUAGES C)

# liburl sources
list(APPEND SOURCES
    "${LIBURL_SRC_DIR}/uri_parse.c"
    "${LIBURL_SRC_DIR}/utf8_c.c"
    "${LIBURL_SRC_DIR}/punycode.c"
    "${LIBURL_SRC_DIR}/encode.c"
)

add_library(url SHARED ${SOURCES})
add_library(urlst STATIC ${SOURCES})
#install(TARGETS url LIBRARY DESTINATION "/usr/local/lib")
#install(TARGETS urlst ARCHIVE DESTINATION "/usr/local/lib")
target_link_libraries(url ${LIBS})
target_link_libraries(urlst ${LIBS})

# examples
file(GLOB_RECURSE SOURCES_EXAMPLE "${LIBURL_SRC_DIR}/example.c")
add_executable(urlexample ${SOURCES_EXAMPLE})
target_link_libraries(urlexample ${LIBS})
target_link_libraries(urlexample url)