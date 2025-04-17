#include <stdlib.h>
#include <stdio.h>

struct CLOD_WorldReader{
    char *path;
};

struct CLOD_WorldReader *CLOD_readWorld(char *path){
    struct CLOD_WorldReader *worldreader = malloc(sizeof(struct CLOD_WorldReader));
}

bool CLOD_readSection(struct CLOD_WorldReader *worldreader, struct CLOD_Section* section) {

}

void CLOD_freeWorld(struct CLOD_WorldReader *worldreader) {
    free(worldreader);
}