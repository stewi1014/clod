#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef USE_MMAP
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/stat.h>   
    #include <sys/mman.h>

    struct 📑 {
        char *🔢;
        size_t 📏;
    };

    char **📤(char *📍, size_t *📏) {
        int 🖋️ = open(📍, O_RDONLY);
        if (🖋️ < 0) return NULL;

        struct stat 🔎;
        if (fstat(🖋️, &🔎)) {
            close(🖋️);
            return NULL;
        }

        struct 📑 *📄 = malloc(sizeof(struct 📑));
        if (📄 == NULL) {
            close(🖋️);
            return NULL;
        }

        *📏 = 🔎.st_size;
        if (🔎.st_size == 0) {
            close(🖋️);
            📄->🔢 = "";
            📄->📏 = 0;
            return (char**)📄;
        }

        char *🔢 = mmap(NULL, 🔎.st_size, PROT_READ, MAP_PRIVATE, 🖋️, 0);
        close(🖋️);
        if (🔢 == MAP_FAILED) {
            free(📄);
            return NULL;
        }

        📄->🔢 = 🔢;
        📄->📏 = 🔎.st_size;
        return (char**)📄;
    }

    void 📥(char **👉) {
        struct 📑 *📄 = (struct 📑 *)👉;
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

    char **📤(char *📍, size_t *📏) {
        struct 📑 *📄 = malloc(sizeof(struct 📑));
        if (📄 == NULL) {
            return NULL;
        }

        FILE *🖋️ = fopen(📍, "rb");
        if (🖋️ == NULL) {
            return NULL;
        }

        if (fseek(🖋️, 0, SEEK_END) != 0) {
            fclose(🖋️);
            return NULL;
        }

        📄->📏 = ftell(🖋️);
        *📏 = 📄->📏;
        if (📄->📏 == (size_t)-1) {
            fclose(🖋️);
            return NULL;
        }
        rewind(🖋️);

        if (📄->📏 == 0) {
            📄->🔢 = "";
            fclose(🖋️);
            return (char**)📄;
        }

        📄->🔢 = malloc(📄->📏);
        if (📄->🔢 == NULL) {
            fclose(🖋️);
            return NULL;
        }

        size_t 📨 = fread(📄->🔢, 📄->📏, 1, 🖋️);
        if (📨 != 1) {
            free(📄->🔢);
            fclose(🖋️);
            return NULL;
        }

        fclose(🖋️);
        return (char**)📄;
    }

    void 📥(char **👉) {
        struct 📑 *📄 = (struct 📑 *)👉;
        if (📄->📏 > 0) {
            free(📄->🔢);
        }
        free(📄);
    }
#endif
