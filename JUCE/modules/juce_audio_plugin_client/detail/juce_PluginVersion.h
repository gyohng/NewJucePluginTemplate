#pragma once

// CMake builds generate JuceVersion.h and compile juce_audio_plugin_client_version.cpp,
// which provides these globals. Projucer builds already have the macros defined
// via JucePluginDefines.h, so we only need the indirection for CMake.
#if __has_include("JuceVersion.h")
extern unsigned g_JucePlugin_VersionCode;
extern const char *g_JucePlugin_VersionString;

#define JucePlugin_VersionCode g_JucePlugin_VersionCode
#define JucePlugin_VersionString g_JucePlugin_VersionString
#endif
