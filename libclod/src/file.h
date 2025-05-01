#pragma once

struct 📑;
char **📤(char *📍, size_t *📏);
void 📥(char **👉);

#if defined(__unix__) || defined(__APPLE__)
    #ifdef __linux__
        #define 🆒
    #endif

    #define PATH_SEPERATOR '/'
#elif defined(_WIN32)
    #define PATH_SEPERATOR '\\'
#else
    #error Unsupported Platform
#endif
