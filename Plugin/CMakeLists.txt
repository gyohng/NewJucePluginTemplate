cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

if (CLANG_WIN32 OR CLANG_WIN64)
    set(CMAKE_C_COMPILER_LAUNCHER "ccache" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER_LAUNCHER "ccache" CACHE STRING "" FORCE)
    set(CMAKE_C_COMPILER "clang" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER "clang++" CACHE STRING "" FORCE)
endif()

if (APPLE)
    set(CMAKE_C_COMPILER_LAUNCHER "ccache" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER_LAUNCHER "ccache" CACHE STRING "" FORCE)
    set(CMAKE_OBJC_COMPILER_LAUNCHER "ccache" CACHE STRING "" FORCE)
    add_compile_options(-Wno-deprecated-anon-enum-enum-conversion -Wno-exceptions -Wno-deprecated-volatile)
endif()

if (NOT USE_XCODE)
    if ("$ENV{CLION_IDE}")
        # do nothing
    elseif (BUILD_MACOS11)
        set(CMAKE_OSX_SYSROOT "$ENV{HOME}/MacOSX-SDKs/MacOSX11.3.sdk" CACHE STRING "" FORCE)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
    else()
        set(CMAKE_OSX_SYSROOT "$ENV{HOME}/MacOSX-SDKs/MacOSX10.13.sdk" CACHE STRING "" FORCE)
        # you may want to remove i386 from the list eventually
        set(CMAKE_OSX_ARCHITECTURES "x86_64;i386" CACHE STRING "" FORCE)
    endif()
    
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "" FORCE)
else()
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
endif()

if (USE_BUILDNUMBER)
    execute_process(COMMAND git rev-list --count HEAD
        OUTPUT_VARIABLE BUILDNUMBER
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
else()
    set(BUILDNUMBER 99999)
endif()

set(DEVELOPER_SIGNATURE "Developer ID Application")

set(SOFTWARE_VERSION 1.0.${BUILDNUMBER})
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/version.txt" ${SOFTWARE_VERSION})

project(MyAudioPlugin VERSION ${SOFTWARE_VERSION} LANGUAGES C CXX)
set(TargetName "${PROJECT_NAME}")

if (NOT MIXER_ROOT_PROJECT)
add_subdirectory(../JUCE JUCE)
endif()

juce_set_vst2_sdk_path("${CMAKE_CURRENT_LIST_DIR}/../JUCE/sdks/vst3sdk")
juce_set_vst3_sdk_path("${CMAKE_CURRENT_LIST_DIR}/../JUCE/sdks/vst3sdk")


if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../JUCE/sdks/aax")
    juce_set_aax_sdk_path("${CMAKE_CURRENT_LIST_DIR}/../JUCE/sdks/aax")
    set(optAAX AAX)
    set(_optAAX _AAX)
    set(HAS_AAX 1)
endif()

string(TIMESTAMP thisyear "%Y")

juce_add_plugin(${TargetName}
    FORMATS ${optAAX} AU AUv3 VST3 VST Standalone

    VERSION ${PROJECT_VERSION}
    # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    ICON_BIG ${CMAKE_CURRENT_LIST_DIR}/icon.png
    # ICON_SMALL ...

    # Change the values below to your own
    PRODUCT_NAME "My Audio Plugin"
    COMPANY_NAME "George Yohng"
    COMPANY_COPYRIGHT "Copyright ©${thisyear} by George Yohng"
    COMPANY_WEBSITE "http://www.yohng.com"

    BUNDLE_ID "com.yohng.plugintemplate"
    PLUGIN_MANUFACTURER_CODE "YOHN"
    PLUGIN_CODE "TMPL"

    AAX_IDENTIFIER "com.yohng.plugintemplate.aax"

    HARDENED_RUNTIME_ENABLED TRUE
    HARDENED_RUNTIME_OPTIONS "com.apple.security.device.audio-input"
    AU_SANDBOX_SAFE TRUE
    APP_SANDBOX_ENABLED TRUE

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE # set also to true if IS_SYNTH is true
    COPY_PLUGIN_AFTER_BUILD FALSE

    MICROPHONE_PERMISSION_ENABLED TRUE
)

target_compile_definitions(${TargetName} PUBLIC
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=1
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_DISABLE_JUCE_VERSION_PRINTING=1
    JUCE_DISABLE_COREGRAPHICS_FONT_SMOOTHING=1
    JUCE_DONT_DECLARE_PROJECTINFO=1
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0
)

juce_generate_juce_header(${TargetName})

# Change this to only include the modules you need
target_link_libraries(${TargetName} PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    # juce::juce_box2d
    juce::juce_core
    juce::juce_cryptography
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    # juce::opengl
    # juce::osc
    # juce::video
)

if (APPLE)
    target_link_libraries(${TargetName} PUBLIC
        "-framework ServiceManagement"
        "-framework Carbon"
        #"-Xarch_i386 -framework -Xarch_i386 Carbon"
    )
endif()

target_sources(${TargetName} PRIVATE
    src/PluginDSP.cpp
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(${TargetName} PROPERTIES FOLDER "")

if (APPLE)
    foreach(ttype All AU AUv3 ${optAAX} Standalone VST VST3)
        set_target_properties(${TargetName}_${ttype} PROPERTIES FOLDER "Build Config")
    endforeach()
elseif(WIN32)
    foreach(ttype All ${optAAX} Standalone VST VST3)
        set_target_properties(${TargetName}_${ttype} PROPERTIES FOLDER "Build Config")
    endforeach()
endif()

set_target_properties(${TargetName} PROPERTIES FOLDER "")

source_group("Source Files" REGULAR_EXPRESSION "src/.*")
source_group(JUCE REGULAR_EXPRESSION "/JUCE/.*|JuceHeader.h|JuceVersion.h")

if (WIN32 AND CMAKE_C_COMPILER_ID STREQUAL Clang)
    add_compile_options(-fms-extensions -fms-compatibility -fms-compatibility-version=19.00)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1 _CRT_SECURE_NO_DEPRECATE=1)
    if (CLANG_WIN32)
        add_compile_options(-Xclang -triple=i686-pc-win32-msvc)
        add_link_options(-Xclang -triple=i686-pc-win32-msvc)
    endif()
    if (CLANG_WIN64)
        add_compile_options(-Xclang -triple=x86_64-pc-windows-msvc)
        add_link_options(-Xclang -triple=x86_64-pc-windows-msvc)
    endif()

    foreach(ttype "" ${_optAAX} _Standalone _VST _VST3)
        target_compile_options(${TargetName}${ttype} PUBLIC
            -O3 -ffast-math -ftree-vectorize
            -fpascal-strings
            -ffunction-sections -fdata-sections
            -fvisibility=hidden -fvisibility-inlines-hidden
            -msse4.1)
    endforeach()
endif()

# Optimisation flags for Windows
if (WIN32 AND NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    foreach(ttype "" ${_optAAX} _Standalone _VST _VST3)
        target_compile_options(${TargetName}${ttype} PUBLIC
            /O2 /fp:fast /Zp16 /GS-)

        if (CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL X86)
            target_compile_options(${TargetName}${ttype} PUBLIC
                /arch:SSE2)
        endif()

        target_link_options(${TargetName}${ttype} PUBLIC
            /OPT:REF)
    endforeach()
endif()


if (APPLE)
    if (USE_XCODE OR "$ENV{CLION_IDE}")
        add_custom_command(
            TARGET ${TargetName}_Standalone POST_BUILD
            COMMAND codesign -v -f -o runtime --timestamp --deep
            --preserve-metadata=entitlements
            --sign "${DEVELOPER_SIGNATURE}"
            --digest-algorithm=sha1,sha256
            "$<TARGET_BUNDLE_CONTENT_DIR:${TargetName}_Standalone>/.."
            VERBATIM)
    endif()

    # Optimisation flags
    foreach(ttype "" _AU _AUv3 _Standalone _VST _VST3 ${_optAAX})
        target_compile_options(${TargetName}${ttype} PUBLIC
            -O3 -ffast-math -ftree-vectorize
            -fpascal-strings
            -ffunction-sections -fdata-sections
            -fvisibility=hidden -fvisibility-inlines-hidden)

        target_link_options(${TargetName}${ttype} PUBLIC
            -ffunction-sections -fdata-sections -Wl,-dead_strip
            -fvisibility=hidden -fvisibility-inlines-hidden)
    endforeach()
endif()

if (APPLE)

    # Note: do not pay attention to the warnings that say that some object files
    # were built for newer macOS version (10.11) than being linked (10.9),
    # this is intended, and this allows the same binary to be reused for diffrent
    # macOS versions.

    if (HAS_AAX)
        # remove arm64 from AAX
        set(aaxarch ${CMAKE_OSX_ARCHITECTURES})
        list(REMOVE_ITEM aaxarch arm64)
        set_target_properties(${TargetName}_AAX PROPERTIES OSX_ARCHITECTURES "${aaxarch}")
    endif()

    # remove i386 from AUv3
    set(auv3arch ${CMAKE_OSX_ARCHITECTURES})
    list(REMOVE_ITEM auv3arch i386)

    set_target_properties(${TargetName}_Standalone PROPERTIES OSX_ARCHITECTURES "${auv3arch}")

    # set 10.11 as the target for AUv3
    set_target_properties(${TargetName}_AUv3 PROPERTIES OSX_ARCHITECTURES "${auv3arch}")

    if (NOT USE_XCODE)
        set_target_properties(${TargetName}_AUv3 PROPERTIES OSX_DEPLOYMENT_TARGET "10.11")

        set_source_files_properties(
            "${CMAKE_CURRENT_LIST_DIR}/../JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_AUv3.mm" PROPERTIES
            COMPILE_FLAGS -mmacosx-version-min=10.11
        )

        # add required flags to compile appex
        target_link_options(${TargetName}_AUv3 PUBLIC -mmacosx-version-min=10.11)
        target_compile_options(${TargetName}_AUv3 PUBLIC -mmacosx-version-min=10.11)
    endif()

    target_link_options(${TargetName}_AUv3 PUBLIC -fapplication-extension -e _NSExtensionMain)
endif()

if (COPY_PLUGINS)
    juce_enable_copy_plugin_step("${TargetName}")
endif()
