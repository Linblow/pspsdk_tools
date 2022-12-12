add_library(${PSPSDK_TOOL_PREFIX_LIB}common STATIC "")

set(HEADERS
  common/elftypes.h
  common/prxtypes.h
  common/sha1.h
  common/types.h
  common/util.h
  glibc/posix/bits/getopt_core.h
  glibc/posix/bits/getopt_ext.h
  glibc/posix/getopt_int.h
  glibc/posix/getopt.h
)
set(SRCS
  common/sha1.c
  common/util.c
  glibc/posix/getopt.c
  glibc/posix/getopt1.c
  # glibc/string/basename.c
)
target_sources(${PSPSDK_TOOL_PREFIX_LIB}common PRIVATE ${HEADERS} ${SRCS})

target_include_directories(${PSPSDK_TOOL_PREFIX_LIB}common PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/glibc/posix
)
