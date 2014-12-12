# (c) 2014, Harm Brouwer

cmake_minimum_required(VERSION 2.8)

project(OSXtlearn)

include(CheckIncludeFile)

set(VERSION "OSX-1.0a")
#set(CMAKE_VERBOSE_MAKEFILE 1)

# setup OSX build
set(PROGNAME OSXtlearn)
set(CMAKE_OSX_ARCHITECTURES "x86_64;i386")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.5")

# X11 for linking
set(X11_INCLUDE_DIR 
        /opt/X11/include
        )
set(X11_LIBRARIES 
        /opt/X11/lib/libX11.dylib
        /opt/X11/lib/libXext.dylib
        # We also want Xaw and Xt
        /opt/X11/lib/libXaw.dylib
        /opt/X11/lib/libXt.dylib
        # debug...
        /opt/X11/lib/libXmu.dylib
        /opt/X11/lib/libICE.dylib
        /opt/X11/lib/libXpm.dylib
        /opt/X11/lib/libSM.dylib
        )
include_directories(${X11_INCLUDE_DIR})
link_libraries(${X11_LIBRARIES})

# compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/opt/X11/include -DXTLEARN")
# source files
set(OSXtlearn_SRC
        src/activate.c
        src/arch.c
        src/arrays.c
        src/cli.c
        src/compute.c
        src/parse.c
        src/subs.c
        src/update.c
        src/weights.c
        src/xtlearn.c
        src/xtlearnActDisp.c
        src/xtlearnAlertBox.c
        src/xtlearnArchDisp.c
        src/xtlearnArchGlue.c
        src/xtlearnErrDisp.c
        src/xtlearnError.c
        src/xtlearnIface.c
        src/xtlearnWtsDisp.c
        )

# MacOSX resources¬
set(OSXtlearn_MACOSX_RESOURCE_FILES
        ${OSXtlearn_SOURCE_DIR}/OSXtlearn.icns)

set_source_files_properties(
        ${OSXtlearn_MACOSX_RESOURCE_FILES}
        PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)

# executable that we want to build
add_executable(${PROGNAME} MACOSX_BUNDLE 
        ${OSXtlearn_SRC}
        ${OSXtlearn_MACOSX_RESOURCE_FILES})
target_link_libraries(${PROGNAME} ${X11_LIBRARY})
install(TARGETS ${PROGNAME}
        BUNDLE DESTINATION ${OSXtlearn_SOURCE_DIR} COMPONENT Runtime)

# wrap everything into an application bundle
set(MACOSX_BUNDLE_NAME "OSXtlearn")
set(MACOSX_BUNDLE_VERSION ${VERSION})
set(MACOSX_BUNDLE_ICON_FILE "OSXtlearn.icns")
set(MACOSX_BUNDLE_GUI_IDENTIFIER "de.uni-saarland.osxtlearn")

set(DIRS ${X11_LIBRARY_PATH})
install(CODE 
        "include(BundleUtilities) 
        fixup_bundle(\"${APP}\" \"\" \"${DIRS}\")"
        COMPONENT Runtime)
set(CPACK_BINARY_DRAGNDROP "ON")
include(CPack)
