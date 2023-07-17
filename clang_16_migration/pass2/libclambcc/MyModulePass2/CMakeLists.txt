# Copyright (C) 2021 Cisco Systems, Inc. and/or its affiliates. All rights reserved.

#
# The clambcc_mymodulepass2 object library
#
add_library(clambcc_mymodulepass2_obj OBJECT)
target_sources(clambcc_mymodulepass2_obj
    PRIVATE
        MyModulePass2.cpp
)

target_include_directories(clambcc_mymodulepass2_obj
    PRIVATE
        ${CMAKE_BINARY_DIR} # For clambc-version.h (generated file)
        .                   # For Common/clambc.h
        Common              # For clambc.h  #TODO: change all passes to use "Common" and then delete this line.
        ${LLVM_INCLUDE_DIRS}
)

set_target_properties(clambcc_mymodulepass2_obj PROPERTIES COMPILE_FLAGS "${WARNCXXFLAGS}")

#
# For testing
#
#target_compile_definitions(clambcc_mymodulepass2_obj -DLOG_BEFORE_AFTER=1)

#
# The clambcc_mymodulepass2 shared library.
#
add_library( clambcc_mymodulepass2 SHARED )
target_link_libraries( clambcc_mymodulepass2
    PUBLIC
        clambcc_mymodulepass2_obj )
set_target_properties( clambcc_mymodulepass2 PROPERTIES
    VERSION ${LIBCLAMBC_VERSION}
    SOVERSION ${LIBCLAMBC_SOVERSION} )

target_link_directories(clambcc_mymodulepass2 PRIVATE ${LLVM_LIBRARY_DIRS})
target_link_libraries(clambcc_mymodulepass2 PUBLIC ${LLVM_LIBS})

if(WIN32)
    install(TARGETS clambcc_mymodulepass2 DESTINATION .)

    # Also install shared library (DLL) dependencies
    install(CODE [[
        file(GET_RUNTIME_DEPENDENCIES
            LIBRARIES
                $<TARGET_FILE:clambcc_mymodulepass2>
            RESOLVED_DEPENDENCIES_VAR _r_deps
            UNRESOLVED_DEPENDENCIES_VAR _u_deps
            DIRECTORIES
                ${LLVM_LIBRARY_DIRS}
        )
        foreach(_file ${_r_deps})
            string(TOLOWER ${_file} _file_lower)
            if(NOT ${_file_lower} MATCHES "c:[\\/]windows[\\/]system32.*")
                file(INSTALL
                    DESTINATION "${CMAKE_INSTALL_PREFIX}"
                    TYPE SHARED_LIBRARY
                    FOLLOW_SYMLINK_CHAIN
                    FILES "${_file}"
                )
            endif()
        endforeach()
        #message("UNRESOLVED_DEPENDENCIES_VAR: ${_u_deps}")
    ]])
else()
    install(TARGETS clambcc_mymodulepass2 DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()



