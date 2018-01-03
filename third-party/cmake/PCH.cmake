#.rst:
# CMakePCH
# --------
#
# Defines following functions:
#
# target_precompiled_header(target [...] header
#                          [REUSE reuse_target])
#
# Uses given header as precompiled header for given target.
#
# Optionally it may reuse existing compiled header object from other target, so
# it is precompiled just once. Both targets need to have same compiler
# arguments otherwise compilation will fail.

# Author: Adam Strzelecki <ono@java.pl>
# Copyright (c) 2014-2015 Adam Strzelecki. All rights reserved.
# This code is licensed under the MIT License, see README.md.

include(CMakeParseArguments)

define_property(TARGET PROPERTY PCH_LANG
  BRIEF_DOCS "PCH language"
  FULL_DOCS "PCH language"
)

define_property(TARGET PROPERTY PCH_OUTPUT
  BRIEF_DOCS "Output path for the PCH file"
  FULL_DOCS "Output path for the PCH file"
)

define_property(TARGET PROPERTY PCH_HEADER
  BRIEF_DOCS "Path that should be included to use the PCH file"
  FULL_DOCS "Path that should be included to use the PCH file"
)

define_property(TARGET PROPERTY PCH_SOURCE
  BRIEF_DOCS "Real path for the PCH header"
  FULL_DOCS "Real path for the PCH header"
)

set(PCH_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})

function(_add_precompiled_header target pch_target header reuse_pch_target)
    get_property(lang SOURCE ${header} PROPERTY LANGUAGE)

    if (NOT lang)
        set(lang CXX)
    endif()

    if(NOT MSVC AND
        NOT CMAKE_COMPILER_IS_GNU${lang} AND
        NOT CMAKE_${lang}_COMPILER_ID STREQUAL "GNU" AND
        NOT CMAKE_${lang}_COMPILER_ID STREQUAL "Clang" AND
        NOT CMAKE_${lang}_COMPILER_ID STREQUAL "AppleClang"
        )
        message(WARNING "Precompiled headers not supported for ${CMAKE_${lang}_COMPILER_ID}")
        return()
    endif()

    if(lang STREQUAL C)
        set(header_type "c-header")
    elseif(lang STREQUAL CXX)
        set(header_type "c++-header")
    else()
        message(WARNING "Unknown header type for language ${lang}")
        set(header_type "c++-header")
    endif()

    if(NOT TARGET "${target}")
        message(SEND_ERROR "Target \"${target}\" does not exist.")
        return()
    endif()

    if(CMAKE_${lang}_COMPILER_ID STREQUAL "GNU")
        set(pch_output_extension .gch)
    else()
        set(pch_output_extension .pch)
    endif()

    if(NOT IS_ABSOLUTE ${header})
        set(header_path ${CMAKE_CURRENT_SOURCE_DIR}/${header})
    else()
        set(header_path ${header})
    endif()

    get_filename_component(header_name ${header} NAME)

    set(pch_source ${CMAKE_CURRENT_BINARY_DIR}/x-${header_name})
    set(pch_temporary ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${pch_target}.dir/x-${header_name}${CMAKE_${lang}_OUTPUT_EXTENSION})
    set(pch_header ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${pch_target}.dir/x-${header_name})
    set(pch_output ${pch_header}${pch_output_extension})

    set(pch_sources "")
    list(APPEND pch_sources ${header_path})

    if(MSVC)
        # ensure pdb goes to the same location, otherwise we get C2859
        file(TO_NATIVE_PATH
            "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${target}.dir"
            pdb_dir
        )
        # /Yc - create precompiled header
		# /Fp - specify output filename
        # /Fd - specify directory for pdb output
        set(flags "/Yc${header_path} /Fp${pch_output} /Fd${pdb_dir}\\")
    else()
        set(flags "-x ${header_type} -fpch-preprocess")
    endif()

    set_source_files_properties(
      ${pch_source}
      PROPERTIES
      LANGUAGE ${lang}
      COMPILE_FLAGS ${flags}
    )

    add_library(${pch_target} OBJECT ${pch_source})

	if(NOT MSVC)
        set_property(TARGET ${pch_target} PROPERTY RULE_LAUNCH_COMPILE ${PCH_CMAKE_DIR}/launch-without-ccache)

        add_custom_command(OUTPUT ${pch_output}
          COMMAND ${CMAKE_COMMAND} -E create_symlink ${pch_temporary} ${pch_output}
          DEPENDS ${pch_target}
        )
	endif()

    set_target_properties(
      ${pch_target} PROPERTIES
      LINKER_LANGUAGE ${lang}
      FOLDER PCH
      PCH_LANG ${lang}
      PCH_OUTPUT ${pch_output}
      PCH_SOURCE ${pch_source}
      PCH_HEADER ${pch_header}
    )

    if(reuse_pch_target)
        get_target_property(reuse_pch_source ${reuse_pch_target} PCH_SOURCE)
        list(APPEND pch_sources ${reuse_pch_source})

        if(CMAKE_${lang}_COMPILER_ID STREQUAL "Clang" OR CMAKE_${lang}_COMPILER_ID STREQUAL "AppleClang")
            _use_pch_for_target(${pch_target} ${reuse_pch_target})
        endif()
    endif()

    add_custom_command(
      OUTPUT ${pch_source}
      COMMAND mkheader
      ARGS 1 ${pch_source} ${pch_sources}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS mkheader ${pch_sources}
    )

    get_target_property(deps ${target} LINK_LIBRARIES)
    if(deps)
        foreach(dep ${deps})
            if(TARGET "${dep}")
                add_dependencies(${pch_target} ${dep})
            endif()
        endforeach()
    endif()

    #get_target_property(define_symbol ${target} DEFINE_SYMBOL)
    #if(define_symbol)
    #    set_property(TARGET ${pch_target} APPEND PROPERTY COMPILE_DEFINITIONS ${define_symbol})
    #endif()

    get_target_property(pic ${target} POSITION_INDEPENDENT_CODE)
    if(pic)
        get_target_property(targetType ${target} TYPE)
        if (targetType STREQUAL "EXECUTABLE" AND CMAKE_${lang}_COMPILE_OPTIONS_PIE)
            set(picFlags "${CMAKE_${lang}_COMPILE_OPTIONS_PIE}")
        elseif (CMAKE_${lang}_COMPILE_OPTIONS_PIC)
            set(picFlags "${CMAKE_${lang}_COMPILE_OPTIONS_PIC}")
        endif()
        set_property(TARGET ${pch_target} APPEND_STRING PROPERTY COMPILE_FLAGS "${picFlags} ")
    endif()
endfunction()

function(_use_pch_for_target target pch_target)
    get_target_property(pch_output ${pch_target} PCH_OUTPUT)
    get_target_property(pch_source ${pch_target} PCH_SOURCE)
    get_target_property(pch_lang ${pch_target} PCH_LANG)

    get_target_property(sources ${target} SOURCES)
    foreach(source ${sources})
      if(source MATCHES "\\.cpp$")
        set_source_files_properties(${source} OBJECT_DEPENDS ${pch_output})
      endif()
    endforeach()

    get_target_property(flags ${target} COMPILE_FLAGS)
    if(MSVC)
	    add_dependencies(${target} ${pch_target})

        #get_filename_component(win_header "${header}" NAME)
        #file(TO_NATIVE_PATH "${target_dir}/${header}.pch" win_pch)
        # /Yu - use given include as precompiled header
        # /Fp - exact location for precompiled header
        # /FI - force include of precompiled header
        set(flags "/Yu${pch_source} /Fp${pch_output} /FI${pch_source}")
    elseif(CMAKE_${pch_lang}_COMPILER_ID STREQUAL "Clang")
        set(flags "-include-pch ${pch_output}")
    else()
        get_target_property(pch_header ${pch_target} PCH_HEADER)
        set(flags "-include ${pch_header}")
    endif()
    set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS "${flags} ")
endfunction()

function(target_precompiled_header) # target [...] header
                                    # [REUSE reuse_target]
    cmake_parse_arguments(ARGS "" "REUSE" "" ${ARGN})

    list(GET ARGS_UNPARSED_ARGUMENTS 0 target)
    list(REMOVE_AT ARGS_UNPARSED_ARGUMENTS 0)

    if(ARGS_UNPARSED_ARGUMENTS)
        list(GET ARGS_UNPARSED_ARGUMENTS 0 header)
        list(REMOVE_AT ARGS_UNPARSED_ARGUMENTS 0)

        if(ARGS_REUSE)
            set(reuse_pch_target "${ARGS_REUSE}.pch")
        else()
            set(reuse_pch_target "")
        endif()

        set(pch_target ${target}.pch)
        _add_precompiled_header(${target} ${pch_target} ${header} "${reuse_pch_target}")
    elseif(ARGS_REUSE)
        set(pch_target ${ARGS_REUSE}.pch)
    endif()

    _use_pch_for_target(${target} ${pch_target})
endfunction()
