# SPDX-FileCopyrightText: Public domain
# SPDX-License-Identifier: BSD-3-Clause

set(TARGET1 ${PSPSDK_TOOL_PREFIX_TOOL}bin2c)
set(TARGET2 ${PSPSDK_TOOL_PREFIX_TOOL}bin2s)

add_executable(${TARGET1} bin2cs.c)
set_target_properties(${TARGET1} PROPERTIES OUTPUT_NAME bin2c)
target_compile_definitions(${TARGET1} PRIVATE BIN2C=1)

add_executable(${TARGET2} bin2cs.c)
set_target_properties(${TARGET2} PROPERTIES OUTPUT_NAME bin2s)
target_compile_definitions(${TARGET2} PRIVATE BIN2S=1)

install(
  TARGETS ${TARGET1} ${TARGET2}
  EXPORT ${PSPSDK_TOOL_EXPORT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
