#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "file.h"

#ifdef 🆒
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/stat.h>   
    #include <sys/mman.h>

    struct 📑 {
        char *🔢;
        size_t 📏;
    };

    char **📤(const char *📍, size_t *📏) {
        const int 🖋️ = open(📍, O_RDONLY);
        if (🖋️ < 0) return nullptr;

        struct stat 🔎;
        if (fstat(🖋️, &🔎)) {
            close(🖋️);
            return nullptr;
        }

        struct 📑 *📄 = malloc(sizeof(struct 📑));
        if (📄 == nullptr) {
            close(🖋️);
            return nullptr;
        }

        *📏 = 🔎.st_size;
        if (🔎.st_size == 0) {
            close(🖋️);
            📄->🔢 = "";
            📄->📏 = 0;
            return (char**)📄;
        }

        char *🔢 = mmap(nullptr, 🔎.st_size, PROT_READ, MAP_PRIVATE, 🖋️, 0);
        close(🖋️);
        if (🔢 == MAP_FAILED) {
            free(📄);
            return nullptr;
        }

        📄->🔢 = 🔢;
        📄->📏 = 🔎.st_size;
        return (char**)📄;
    }

    void 📥(char **👉) {
        const auto 📄 = (struct 📑 *)👉;
        if (📄->📏 > 0) {
            munmap(📄->🔢, 📄->📏);
        }
        free(📄);
    }
#else
    struct 📑 {
        char *🔢;
        size_t 📏;
    };

    char **📤(const char *📍, size_t *📏) {
        struct 📑 *📄 = malloc(sizeof(struct 📑));
        if (📄 == nullptr) {
            return nullptr;
        }

        FILE *🖋️ = fopen(📍, "rb");
        if (🖋️ == nullptr) {
            return nullptr;
        }

        if (fseek(🖋️, 0, SEEK_END) != 0) {
            fclose(🖋️);
            return nullptr;
        }

        📄->📏 = ftell(🖋️);
        *📏 = 📄->📏;
        if (📄->📏 == (size_t)-1) {
            fclose(🖋️);
            return nullptr;
        }
        rewind(🖋️);

        if (📄->📏 == 0) {
            📄->🔢 = "";
            fclose(🖋️);
            return (char**)📄;
        }

        📄->🔢 = malloc(📄->📏);
        if (📄->🔢 == nullptr) {
            fclose(🖋️);
            return nullptr;
        }

        const size_t 📨 = fread(📄->🔢, 📄->📏, 1, 🖋️);
        if (📨 != 1) {
            free(📄->🔢);
            fclose(🖋️);
            return nullptr;
        }

        fclose(🖋️);
        return (char**)📄;
    }

    void 📥(char **👉) {
        const auto 📄 = (struct 📑 *)👉;
        if (📄->📏 > 0) {
            free(📄->🔢);
        }
        free(📄);
    }
#endif
