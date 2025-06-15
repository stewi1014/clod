#cmakedefine01 DISABLE_CONCURRENCY

#cmakedefine01 HAVE_WINDOWS
#cmakedefine01 HAVE_UNIX

#cmakedefine01 HAVE_MREMAP
#cmakedefine01 HAVE_OFD_LOCKS

#ifdef __AVX512BW__
#define HAVE_AVX_512 1
#endif

#ifdef __AVX2__
#define HAVE_AVX_256 1
#endif

#cmakedefine01 HAVE_PTHREAD_MUTEX
#cmakedefine01 HAVE_THREADS_MUTEX
