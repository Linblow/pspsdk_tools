# PSPSDK tools

A bunch of tools for developing on the Sony PSP:

- `build-exports` with new options added
- `fixup-imports`
- `kprxgen` (not done yet, equivalent to `prxgen`)
- `prxgen`
- `pack-pbp` remade from scratch with new options (e.g. quiet option)
- `sign-np` (Hykem's)

## Build and install

Open a terminal in the project's root directory and type:

```shell
# Build the tools and install them to the local PSPDEV directory.
# The existing tools, if any, will be overwritten.
./b.sh all install

# Show the other available commands.
./b.sh help
```

A CMake import script will be installed to `$PSPDEV/lib/cmake/pspsdk-tools-targets.cmake`.  
Other projects can then import the tools targets:

```cmake
# Required for the CMake scripts that include(pspsdk-tools-targets)
list(APPEND CMAKE_MODULE_PATH
  "$ENV{PSPDEV}/lib/cmake"
  "$ENV{PSPDEV}/psp/lib/cmake"
)

# The pspsdk tools targets must be installed locally.
include(pspsdk-tools-targets)

# For example, build the exports C file.
get_target_property(EXEC pspsdk::build-exports LOCATION)
add_custom_command(
  OUTPUT exports.c
  DEPENDS exports.exp
  COMMAND ${EXEC} -b --uofw exports.exp > exports.c
)
```
