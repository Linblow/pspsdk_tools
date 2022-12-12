# SPDX-FileCopyrightText: Public domain
# SPDX-License-Identifier: BSD-3-Clause

set(TARGET ${PSPSDK_TOOL_PREFIX_TOOL}build-exports)
add_executable(${TARGET} "")

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME psp-build-exports)
target_sources(${TARGET} PRIVATE psp-build-exports.c)
target_link_libraries(${TARGET} PRIVATE ${PSPSDK_TOOL_PREFIX_LIB}common)

install(
  TARGETS ${TARGET}
  EXPORT ${PSPSDK_TOOL_EXPORT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

