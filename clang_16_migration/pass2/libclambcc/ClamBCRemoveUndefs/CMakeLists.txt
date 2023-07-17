# Copyright (C) 2021 Cisco Systems, Inc. and/or its affiliates. All rights reserved.

#
# The clambcc_clambcremoveundefs object library
#
add_library(clambcc_clambcremoveundefs_obj OBJECT)
target_sources(clambcc_clambcremoveundefs_obj
    PRIVATE
        ClamBCRemoveUndefs.cpp
)

target_include_directories(clambcc_clambcremoveundefs_obj
    PRIVATE
        ${CMAKE_BINARY_DIR} # For clambc-version.h (generated file)
        .                   # For Common/clambc.h
        ..                  # For clambc.h  #TODO: change all passes to use "Common" and then delete this line.
        ${LLVM_INCLUDE_DIRS}
)

set_target_properties(clambcc_clambcremoveundefs_obj PROPERTIES COMPILE_FLAGS "${WARNCXXFLAGS}")

#
# For testing
#
#target_compile_definitions(clambcc_clambcremoveundefs_obj -DLOG_BEFORE_AFTER=1)

#
# The clambcc_clambcremoveundefs shared library.
#
add_library( clambcc_clambcremoveundefs SHARED )
target_link_libraries( clambcc_clambcremoveundefs
    PUBLIC
        clambcc_clambcremoveundefs_obj )
set_target_properties( clambcc_clambcremoveundefs PROPERTIES
    VERSION ${LIBCLAMBC_VERSION}
    SOVERSION ${LIBCLAMBC_SOVERSION} )

target_link_directories(clambcc_clambcremoveundefs PRIVATE ${LLVM_LIBRARY_DIRS})
target_link_libraries(clambcc_clambcremoveundefs PUBLIC ${LLVM_LIBS})

if(WIN32)
    install(TARGETS clambcc_clambcremoveundefs DESTINATION .)

    # Also install shared library (DLL) dependencies
    install(CODE [[
        file(GET_RUNTIME_DEPENDENCIES
            LIBRARIES
                $<TARGET_FILE:clambcc_clambcremoveundefs>
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
    install(TARGETS clambcc_clambcremoveundefs DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()



