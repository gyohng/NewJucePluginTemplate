// Derived from os_version_check.c 

#ifdef __APPLE__

#include <TargetConditionals.h>
#include <AvailabilityMacros.h>

#if !defined(MAC_OS_VERSION_10_15) || !MAC_OS_VERSION_10_15

#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// These three variables hold the host's OS version.
static int32_t GlobalMajor, GlobalMinor, GlobalSubminor;
static dispatch_once_t DispatchOnceCounter;
static dispatch_once_t CompatibilityDispatchOnceCounter;

// _availability_version_check darwin API support.
typedef uint32_t dyld_platform_t;

typedef struct {
  dyld_platform_t platform;
  uint32_t version;
} dyld_build_version_t;

typedef bool (*AvailabilityVersionCheckFuncTy)(uint32_t count,
                                               dyld_build_version_t versions[]);

static AvailabilityVersionCheckFuncTy AvailabilityVersionCheck;

// We can't include <CoreFoundation/CoreFoundation.h> directly from here, so
// just forward declare everything that we need from it.

namespace compatTypes {
typedef const void *CFDataRef, *CFAllocatorRef, *CFPropertyListRef,
    *CFStringRef, *CFDictionaryRef, *CFTypeRef, *CFErrorRef;

#if __LLP64__
typedef unsigned long long CFTypeID;
typedef unsigned long long CFOptionFlags;
typedef signed long long CFIndex;
#else
typedef unsigned long CFTypeID;
typedef unsigned long CFOptionFlags;
typedef signed long CFIndex;
#endif

typedef unsigned char UInt8;
//typedef _Bool Boolean;
typedef CFIndex CFPropertyListFormat;
typedef uint32_t CFStringEncoding;

// kCFStringEncodingASCII analog.
#define CF_STRING_ENCODING_ASCII 0x0600
// kCFStringEncodingUTF8 analog.
#define CF_STRING_ENCODING_UTF8 0x08000100
#define CF_PROPERTY_LIST_IMMUTABLE 0

}

typedef CFDataRef (*CFDataCreateWithBytesNoCopyFuncTy)(CFAllocatorRef,
                                                       const UInt8 *, CFIndex,
                                                       CFAllocatorRef);
typedef CFPropertyListRef (*CFPropertyListCreateWithDataFuncTy)(
    CFAllocatorRef, CFDataRef, CFOptionFlags, CFPropertyListFormat *,
    CFErrorRef *);
typedef CFPropertyListRef (*CFPropertyListCreateFromXMLDataFuncTy)(
    CFAllocatorRef, CFDataRef, CFOptionFlags, CFStringRef *);
typedef CFStringRef (*CFStringCreateWithCStringNoCopyFuncTy)(CFAllocatorRef,
                                                             const char *,
                                                             CFStringEncoding,
                                                             CFAllocatorRef);
typedef const void *(*CFDictionaryGetValueFuncTy)(CFDictionaryRef,
                                                  const void *);
typedef CFTypeID (*CFGetTypeIDFuncTy)(CFTypeRef);
typedef CFTypeID (*CFStringGetTypeIDFuncTy)(void);
typedef Boolean (*CFStringGetCStringFuncTy)(CFStringRef, char *, CFIndex,
                                            CFStringEncoding);
typedef void (*CFReleaseFuncTy)(CFTypeRef);


static void _initializeAvailabilityCheck(bool LoadPlist) {
  if (AvailabilityVersionCheck && !LoadPlist) {
    // New API is supported and we're not being asked to load the plist,
    // exit early!
    return;
  }

  if (AvailabilityVersionCheck && !LoadPlist) {
    // New API is supported and we're not being asked to load the plist,
    // exit early!
    return;
  }
  // Still load the PLIST to ensure that the existing calls to
  // __isOSVersionAtLeast still work even with new compiler-rt and old OSes.

  // Load CoreFoundation dynamically
  const void *NullAllocator = dlsym(RTLD_DEFAULT, "kCFAllocatorNull");
  if (!NullAllocator)
    return;
  const CFAllocatorRef AllocatorNull = *(const CFAllocatorRef *)NullAllocator;
  CFDataCreateWithBytesNoCopyFuncTy CFDataCreateWithBytesNoCopyFunc =
      (CFDataCreateWithBytesNoCopyFuncTy)dlsym(RTLD_DEFAULT,
                                               "CFDataCreateWithBytesNoCopy");
  if (!CFDataCreateWithBytesNoCopyFunc)
    return;
  CFPropertyListCreateWithDataFuncTy CFPropertyListCreateWithDataFunc =
      (CFPropertyListCreateWithDataFuncTy)dlsym(RTLD_DEFAULT,
                                                "CFPropertyListCreateWithData");
// CFPropertyListCreateWithData was introduced only in macOS 10.6+, so it
// will be nullptr on earlier OS versions.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  CFPropertyListCreateFromXMLDataFuncTy CFPropertyListCreateFromXMLDataFunc =
      (CFPropertyListCreateFromXMLDataFuncTy)dlsym(
          RTLD_DEFAULT, "CFPropertyListCreateFromXMLData");
#pragma clang diagnostic pop
  // CFPropertyListCreateFromXMLDataFunc is deprecated in macOS 10.10, so it
  // might be nullptr in future OS versions.
  if (!CFPropertyListCreateWithDataFunc && !CFPropertyListCreateFromXMLDataFunc)
    return;
  CFStringCreateWithCStringNoCopyFuncTy CFStringCreateWithCStringNoCopyFunc =
      (CFStringCreateWithCStringNoCopyFuncTy)dlsym(
          RTLD_DEFAULT, "CFStringCreateWithCStringNoCopy");
  if (!CFStringCreateWithCStringNoCopyFunc)
    return;
  CFDictionaryGetValueFuncTy CFDictionaryGetValueFunc =
      (CFDictionaryGetValueFuncTy)dlsym(RTLD_DEFAULT, "CFDictionaryGetValue");
  if (!CFDictionaryGetValueFunc)
    return;
  CFGetTypeIDFuncTy CFGetTypeIDFunc =
      (CFGetTypeIDFuncTy)dlsym(RTLD_DEFAULT, "CFGetTypeID");
  if (!CFGetTypeIDFunc)
    return;
  CFStringGetTypeIDFuncTy CFStringGetTypeIDFunc =
      (CFStringGetTypeIDFuncTy)dlsym(RTLD_DEFAULT, "CFStringGetTypeID");
  if (!CFStringGetTypeIDFunc)
    return;
  CFStringGetCStringFuncTy CFStringGetCStringFunc =
      (CFStringGetCStringFuncTy)dlsym(RTLD_DEFAULT, "CFStringGetCString");
  if (!CFStringGetCStringFunc)
    return;
  CFReleaseFuncTy CFReleaseFunc =
      (CFReleaseFuncTy)dlsym(RTLD_DEFAULT, "CFRelease");
  if (!CFReleaseFunc)
    return;

  const char *PListPath = "/System/Library/CoreServices/SystemVersion.plist";

#if TARGET_OS_SIMULATOR
  const char *PListPathPrefix = getenv("IPHONE_SIMULATOR_ROOT");
  if (!PListPathPrefix)
    return;
  char FullPath[strlen(PListPathPrefix) + strlen(PListPath) + 1];
  strcpy(FullPath, PListPathPrefix);
  strcat(FullPath, PListPath);
  PListPath = FullPath;
#endif
  FILE *PropertyList = fopen(PListPath, "r");
  if (!PropertyList)
    return;

  // Dynamically allocated stuff.
  CFDictionaryRef PListRef = nullptr;
  CFDataRef FileContentsRef = nullptr;
  UInt8 *PListBuf = nullptr;

  {
  fseek(PropertyList, 0, SEEK_END);
  long PListFileSize = ftell(PropertyList);
  if (PListFileSize < 0)
    goto Fail;
  rewind(PropertyList);

  PListBuf = (unsigned char *)malloc((size_t)PListFileSize);
  if (!PListBuf)
    goto Fail;
  
  size_t NumRead = fread(PListBuf, 1, (size_t)PListFileSize, PropertyList);
  if (NumRead != (size_t)PListFileSize)
    goto Fail;

  // Get the file buffer into CF's format. We pass in a null allocator here *
  // because we free PListBuf ourselves
  FileContentsRef = (*CFDataCreateWithBytesNoCopyFunc)(
      nullptr, PListBuf, (CFIndex)NumRead, AllocatorNull);
  if (!FileContentsRef)
    goto Fail;

  if (CFPropertyListCreateWithDataFunc)
    PListRef = (CFDictionaryRef)(*CFPropertyListCreateWithDataFunc)(
        nullptr, FileContentsRef, CF_PROPERTY_LIST_IMMUTABLE, nullptr, nullptr);
  else
    PListRef = (CFDictionaryRef)(*CFPropertyListCreateFromXMLDataFunc)(
        nullptr, FileContentsRef, CF_PROPERTY_LIST_IMMUTABLE, nullptr);
  if (!PListRef)
    goto Fail;

  CFStringRef ProductVersion = (*CFStringCreateWithCStringNoCopyFunc)(
      nullptr, "ProductVersion", CF_STRING_ENCODING_ASCII, AllocatorNull);
  if (!ProductVersion)
    goto Fail;
  

  CFTypeRef OpaqueValue = (*CFDictionaryGetValueFunc)(PListRef, ProductVersion);
  (*CFReleaseFunc)(ProductVersion);
  if (!OpaqueValue ||
      (*CFGetTypeIDFunc)(OpaqueValue) != (*CFStringGetTypeIDFunc)())
    goto Fail;

  char VersionStr[32];
  if (!(*CFStringGetCStringFunc)((CFStringRef)OpaqueValue, VersionStr,
                                 sizeof(VersionStr), CF_STRING_ENCODING_UTF8))
    goto Fail;
  sscanf(VersionStr, "%d.%d.%d", &GlobalMajor, &GlobalMinor, &GlobalSubminor);
  }
  
Fail:
  if (PListRef)
    (*CFReleaseFunc)(PListRef);
  if (FileContentsRef)
    (*CFReleaseFunc)(FileContentsRef);
  free(PListBuf);
  fclose(PropertyList);
}

// Find and parse the SystemVersion.plist file.
static void compatibilityInitializeAvailabilityCheck(void *Unused) {
  (void)Unused;
  _initializeAvailabilityCheck(/*LoadPlist=*/true);
}

static void initializeAvailabilityCheck(void *Unused) {
  (void)Unused;
  _initializeAvailabilityCheck(/*LoadPlist=*/false);
}

// This old API entry point is no longer used by Clang for Darwin. We still need
// to keep it around to ensure that object files that reference it are still
// usable when linked with new compiler-rt.
__attribute__((weak)) 
extern "C" int32_t __isOSVersionAtLeast(int32_t Major, int32_t Minor, int32_t Subminor) {
  // Populate the global version variables, if they haven't already.
  dispatch_once_f(&CompatibilityDispatchOnceCounter, nullptr,
                  compatibilityInitializeAvailabilityCheck);

  if (Major < GlobalMajor)
    return 1;
  if (Major > GlobalMajor)
    return 0;
  if (Minor < GlobalMinor)
    return 1;
  if (Minor > GlobalMinor)
    return 0;
  return Subminor <= GlobalSubminor;
}

static inline uint32_t ConstructVersion(uint32_t Major, uint32_t Minor,
                                        uint32_t Subminor) {
  return ((Major & 0xffff) << 16) | ((Minor & 0xff) << 8) | (Subminor & 0xff);
}

__attribute__((weak)) 
extern "C" int32_t __isPlatformVersionAtLeast(uint32_t Platform, uint32_t Major,
                                   uint32_t Minor, uint32_t Subminor) {
  dispatch_once_f(&DispatchOnceCounter, nullptr, initializeAvailabilityCheck);

  if (!AvailabilityVersionCheck) {
    return __isOSVersionAtLeast(Major, Minor, Subminor);
  }
  dyld_build_version_t Versions[] = {
      {Platform, ConstructVersion(Major, Minor, Subminor)}};
  return AvailabilityVersionCheck(1, Versions);
}

 __attribute__((weak))
extern "C" bool _availability_version_check(uint32_t count, dyld_build_version_t versions[]) {
  dispatch_once_f(&DispatchOnceCounter, nullptr, initializeAvailabilityCheck);
  return AvailabilityVersionCheck(count, versions);
}

#endif // !defined(MAC_OS_VERSION_10_15) || !MAC_OS_VERSION_10_15

#else
// Silence an empty translation unit warning.
typedef int unused;

#endif
