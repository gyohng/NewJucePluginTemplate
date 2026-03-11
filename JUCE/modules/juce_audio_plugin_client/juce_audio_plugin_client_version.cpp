#if __has_include("JuceVersion.h")
 #include "JuceVersion.h"
#elif __has_include("JucePluginDefines.h")
 #include "JucePluginDefines.h"
#else
 #error "Neither JuceVersion.h (CMake) nor JucePluginDefines.h (Projucer) found"
#endif

extern unsigned g_JucePlugin_VersionCode;
extern const char *g_JucePlugin_VersionString;

unsigned g_JucePlugin_VersionCode = JucePlugin_VersionCode;
const char *g_JucePlugin_VersionString = JucePlugin_VersionString;
