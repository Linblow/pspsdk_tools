# SPDX-FileCopyrightText: Public domain
# SPDX-License-Identifier: BSD-3-Clause

set(TARGET ${PSPSDK_TOOL_PREFIX_TOOL}sign-np)
add_executable(${TARGET} "")

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME psp-sign-np)

set(HEADERS
  libkirk/aes.h
  libkirk/amctrl.h
  libkirk/key_vault.h
  libkirk/kirk_engine.h
  libkirk/psp_headers.h
  libkirk/sha1.h
  eboot.h
  isoreader.h
  pgd.h
  sign_np.h
  tlzrc.h
  utils.h
)
set(SRCS
  libkirk/aes.c
  libkirk/amctrl.c
  libkirk/bn.c
  libkirk/ec.c
  libkirk/kirk_engine.c
  libkirk/sha1.c
  eboot.c
  isoreader.c
  pgd.c
  sign_np.c
  tlzrc.c
  utils.c
)
target_sources(${TARGET} PRIVATE ${HEADERS} ${SRCS})
target_link_libraries(${TARGET} PRIVATE z)
target_compile_options(${TARGET} PRIVATE -Wno-unused-function)

install(
  TARGETS ${TARGET}
  EXPORT ${PSPSDK_TOOL_EXPORT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
