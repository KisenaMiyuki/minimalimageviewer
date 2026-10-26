#ifndef JXL_CMS_EXPORT_H
#define JXL_CMS_EXPORT_H

#ifdef JXL_CMS_STATIC_DEFINE
#  define JXL_CMS_EXPORT
#  define JXL_CMS_NO_EXPORT
#else
#  ifndef JXL_CMS_EXPORT
#    if defined(_WIN32) || defined(__CYGWIN__)
#      ifdef JXL_CMS_INTERNAL_LIBRARY_BUILD
#        define JXL_CMS_EXPORT __declspec(dllexport)
#      else
#        define JXL_CMS_EXPORT __declspec(dllimport)
#      endif
#    else
#      define JXL_CMS_EXPORT __attribute__((visibility("default")))
#    endif
#  endif
#  ifndef JXL_CMS_NO_EXPORT
#    define JXL_CMS_NO_EXPORT
#  endif
#endif

#ifndef JXL_CMS_DEPRECATED
#  if defined(_WIN32)
#    define JXL_CMS_DEPRECATED __declspec(deprecated)
#  else
#    define JXL_CMS_DEPRECATED __attribute__((__deprecated__))
#  endif
#endif

#ifndef JXL_CMS_DEPRECATED_EXPORT
#  define JXL_CMS_DEPRECATED_EXPORT JXL_CMS_EXPORT JXL_CMS_DEPRECATED
#endif

#ifndef JXL_CMS_DEPRECATED_NO_EXPORT
#  define JXL_CMS_DEPRECATED_NO_EXPORT JXL_CMS_NO_EXPORT JXL_CMS_DEPRECATED
#endif

#endif /* JXL_CMS_EXPORT_H */
