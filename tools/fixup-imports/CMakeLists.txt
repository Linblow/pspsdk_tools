# SPDX-FileCopyrightText: Public domain
# SPDX-License-Identifier: BSD-3-Clause

set(TARGET ${PSPSDK_TOOL_PREFIX_TOOL}fixup-imports)
add_executable(${TARGET} "")

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME psp-fixup-imports)
target_sources(${TARGET} PRIVATE psp-fixup-imports.c)
target_link_libraries(${TARGET} PRIVATE ${PSPSDK_TOOL_PREFIX_LIB}common)

install(
  TARGETS ${TARGET}
  EXPORT ${PSPSDK_TOOL_EXPORT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

