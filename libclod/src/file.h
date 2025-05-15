#pragma once

struct 📑;
char **📤(const char *📍, size_t *📏);
void 📥(char **👉);

#if defined(__unix__) || defined(__APPLE__)
    #define 🆒
    #define PATH_SEPERATOR '/'
#elif defined(_WIN32)
    #define PATH_SEPERATOR '\\'
#else
    #error Unsupported Platform
#endif
