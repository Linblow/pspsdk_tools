# SPDX-FileCopyrightText: Public domain
# SPDX-License-Identifier: BSD-3-Clause
set(TOOL_MAJOR 1)
set(TOOL_MINOR 0)
set(TOOL_PATCH 0)
set(TOOL_VERSION "${TOOL_MAJOR}.${TOOL_MINOR}.${TOOL_PATCH}")

set(TARGET ${PSPSDK_TOOL_PREFIX_TOOL}pack-pbp)
add_executable(${TARGET} "")

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME pack-pbp)
target_sources(${TARGET} PRIVATE pack-pbp.c)
target_link_libraries(${TARGET} PRIVATE ${PSPSDK_TOOL_PREFIX_LIB}common)
target_compile_definitions(${TARGET} PRIVATE
  TOOL_MAJOR=${TOOL_MAJOR}
  TOOL_MINOR=${TOOL_MINOR}
  TOOL_PATCH=${TOOL_PATCH}
  TOOL_VERSION="${TOOL_VERSION}"
)

install(
  TARGETS ${TARGET}
  EXPORT ${PSPSDK_TOOL_EXPORT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)