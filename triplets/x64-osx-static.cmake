set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES x86_64)
set(VCPKG_OSX_DEPLOYMENT_TARGET 11.5)

if(PORT STREQUAL "mimalloc")
    message("Overriding MI_NO_USE_CXX and MI_USE_CXX for mi-malloc")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS "-DMI_NO_USE_CXX:BOOL=ON")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS ${VCPKG_CMAKE_CONFIGURE_OPTIONS} "-DMI_USE_CXX:BOOL=OFF")
endif()
