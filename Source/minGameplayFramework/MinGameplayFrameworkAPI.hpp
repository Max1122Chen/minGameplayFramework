#pragma once

// Export surface for this SHARED game module. Reflection StaticClass / GetClassName
// follow ME_REFLECTION_MODULE_API, which is aliased to this macro below.
#if defined(_WIN32) && defined(MINGAMEPLAYFRAMEWORK_BUILD)
#    define MINGAMEPLAYFRAMEWORK_API __declspec(dllexport)
#elif defined(_WIN32)
#    define MINGAMEPLAYFRAMEWORK_API __declspec(dllimport)
#else
#    define MINGAMEPLAYFRAMEWORK_API __attribute__((visibility("default")))
#endif

#ifndef ME_REFLECTION_MODULE_API
#    define ME_REFLECTION_MODULE_API MINGAMEPLAYFRAMEWORK_API
#endif
