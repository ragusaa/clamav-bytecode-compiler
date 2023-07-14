target_include_directories(hello_obj
    PRIVATE
        ../libclambcc  # HACK: For Common/clambc.h
        ${LLVM_INCLUDE_DIRS}
)

add_library( Migration MODULE
  src/Migration.cpp

  )

