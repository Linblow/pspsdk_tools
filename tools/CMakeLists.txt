# add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-Wall -Wextra -Wno-stringop-truncation)

add_compile_definitions("$<$<CONFIG:Debug>:DEBUG=1>")

add_subdirectory(build-exports)
add_subdirectory(fixup-imports)
add_subdirectory(kprxgen)
add_subdirectory(pack-pbp)
add_subdirectory(prxgen)
add_subdirectory(sign_np-hykem)

install(
  EXPORT ${PSPSDK_TOOL_EXPORT_NAME}
  DESTINATION lib/cmake
  NAMESPACE ${PSPSDK_TOOL_NAMESPACE}
)
