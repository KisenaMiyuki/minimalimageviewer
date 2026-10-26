#ifndef JXL_EXPORT_H
#define JXL_EXPORT_H

#ifdef JXL_STATIC_DEFINE
#  define JXL_EXPORT
#  define JXL_NO_EXPORT
#else
#  ifndef JXL_EXPORT
#    if defined(_WIN32) || defined(__CYGWIN__)
#      ifdef JXL_INTERNAL_LIBRARY_BUILD
#        define JXL_EXPORT __declspec(dllexport)
#      else
#        define JXL_EXPORT __declspec(dllimport)
#      endif
#    else
#      define JXL_EXPORT __attribute__((visibility("default")))
#    endif
#  endif
#  ifndef JXL_NO_EXPORT
#    if defined(_WIN32) || defined(__CYGWIN__)
#      define JXL_NO_EXPORT
#    else
#      define JXL_NO_EXPORT __attribute__((visibility("hidden")))
#    endif
#  endif
#endif

#ifndef JXL_DEPRECATED
#  if defined(_WIN32)
#    define JXL_DEPRECATED __declspec(deprecated)
#  else
#    define JXL_DEPRECATED __attribute__((__deprecated__))
#  endif
#endif

#ifndef JXL_DEPRECATED_EXPORT
#  define JXL_DEPRECATED_EXPORT JXL_EXPORT JXL_DEPRECATED
#endif

#ifndef JXL_DEPRECATED_NO_EXPORT
#  define JXL_DEPRECATED_NO_EXPORT JXL_NO_EXPORT JXL_DEPRECATED
#endif

#endif /* JXL_EXPORT_H */
