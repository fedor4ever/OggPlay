#if defined(__WINS__)
#define FPM_DEFAULT
#else
#define FPM_ARM
#endif


/* Define to optimize for accuracy over speed. */
#define OPT_ACCURACY

/* Define to enable a fast subband synthesis approximation optimization. */
#define OPT_SSO 

/* Name of package */
#define PACKAGE "libmad"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "support@underbit.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "MPEG Audio Decoder"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "MPEG Audio Decoder 0.15.1b"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libmad"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.15.1b"

/* The size of a `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of a `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* Version number of package */
#define VERSION "0.15.1b"

/* Define as `__inline' if that's what the C compiler calls it, or to nothing
   if it is not supported. */
#define inline __inline


#include <OggShared.h>
