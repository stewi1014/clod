/**
 * Minecraft region file parsing
 */

struct RG_File;
struct RG_File RG_open(char *path);
void RG_close(struct RG_File);
