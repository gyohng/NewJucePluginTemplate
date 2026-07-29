#pragma once

// =============================================================================
// os_workgroup polyfill for building with pre-macOS-11 SDKs (e.g. 10.13)
//
// <os/workgroup.h> and the audio-workgroup additions to CoreAudio/AudioToolbox
// only exist in macOS 11+ SDKs. This header re-declares the small subset JUCE
// needs, using the exact ABI of the real headers, and resolves the functions
// from libSystem at runtime via dlsym. When the binary runs on macOS 11+ the
// real functions are found and audio workgroups behave exactly as if the
// binary had been built with a modern SDK; on older systems the lookups fail
// and everything degrades gracefully to the "no workgroup" code path.
// =============================================================================

#include <os/object.h>
#include <dlfcn.h>
#include <cstdint>

// Same declaration the macOS 11 SDK makes in <os/workgroup_object.h>: an
// os_object class. At runtime, workgroup instances returned by the OS are
// genuine Objective-C os_objects, so the os_retain/os_release macros (which
// expand to retain/release message sends in Objective-C++) work on them.
OS_OBJECT_DECL_CLASS (os_workgroup);

// Layout must match <os/workgroup_base.h> in the macOS 11+ SDK.
#if defined (__LP64__)
 #define JUCE_OS_WORKGROUP_JOIN_TOKEN_OPAQUE_SIZE 36
#else
 #define JUCE_OS_WORKGROUP_JOIN_TOKEN_OPAQUE_SIZE 28
#endif

struct os_workgroup_join_token_opaque_s
{
    uint32_t sig;
    char opaque[JUCE_OS_WORKGROUP_JOIN_TOKEN_OPAQUE_SIZE];
};
typedef struct os_workgroup_join_token_opaque_s os_workgroup_join_token_s;
typedef struct os_workgroup_join_token_opaque_s* os_workgroup_join_token_t;

// Function pointers resolved from libSystem. The warm-up static below makes
// dlsym run during static initialisation rather than on the realtime audio
// thread that first joins a workgroup.
struct juce_polyfill_OsWorkgroupFns
{
    int  (* join)               (os_workgroup_t, os_workgroup_join_token_t) = nullptr;
    void (* leave)              (os_workgroup_t, os_workgroup_join_token_t) = nullptr;
    int  (* maxParallelThreads) (os_workgroup_t, void*) = nullptr;

    static const juce_polyfill_OsWorkgroupFns& get()
    {
        static const juce_polyfill_OsWorkgroupFns fns;
        return fns;
    }

private:
    juce_polyfill_OsWorkgroupFns()
    {
        join               = reinterpret_cast<decltype (join)>               (dlsym (RTLD_DEFAULT, "os_workgroup_join"));
        leave              = reinterpret_cast<decltype (leave)>              (dlsym (RTLD_DEFAULT, "os_workgroup_leave"));
        maxParallelThreads = reinterpret_cast<decltype (maxParallelThreads)> (dlsym (RTLD_DEFAULT, "os_workgroup_max_parallel_threads"));
    }
};

[[maybe_unused]] static const auto& juce_polyfill_osWorkgroupWarmUp = juce_polyfill_OsWorkgroupFns::get();

inline int os_workgroup_join (os_workgroup_t wg, os_workgroup_join_token_t tokenOut)
{
    if (auto* fn = juce_polyfill_OsWorkgroupFns::get().join)
        return fn (wg, tokenOut);

    return 78; // ENOSYS: running on a system without workgroup support
}

inline void os_workgroup_leave (os_workgroup_t wg, os_workgroup_join_token_t token)
{
    if (auto* fn = juce_polyfill_OsWorkgroupFns::get().leave)
        fn (wg, token);
}

inline int os_workgroup_max_parallel_threads (os_workgroup_t wg, void* attr)
{
    if (auto* fn = juce_polyfill_OsWorkgroupFns::get().maxParallelThreads)
        return fn (wg, attr);

    return 0;
}

// =============================================================================
// CoreAudio / AudioToolbox additions that arrived with the macOS 11 SDK.
// Values and layouts match AudioHardware.h / AudioUnitProperties.h.
// =============================================================================

enum
{
    kAudioDevicePropertyIOThreadOSWorkgroup = 0x6F737767 // 'oswg'
};

struct AudioUnitRenderContext
{
    os_workgroup_t workgroup;
    uint32_t reserved[6]; // must be zero
};
typedef struct AudioUnitRenderContext AudioUnitRenderContext;

#if defined (__BLOCKS__)
typedef void (^AURenderContextObserver) (const AudioUnitRenderContext* context);
#endif

enum
{
    kAudioUnitProperty_RenderContextObserver = 60
};
