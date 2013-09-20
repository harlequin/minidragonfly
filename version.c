#include "version.h"

#ifdef GIT_VERSION
const char * build_git_sha =  GIT_VERSION;
#else
const char * build_git_sha = "no-version information";
#endif

const char * build_git_time = __DATE__ " - " __TIME__;
