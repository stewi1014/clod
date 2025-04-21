
#ifdef __unix__
#elif _WIN32
#error Windows is not currently supported. // TODO; windows support
#elif __APPLE__
#error OSX is not currently supported. // TODO; OSX support
#else
#error Platform is not currently supported.
#endif

#define scan_region_pos(filepath, region_x, region_y) \
    sscanf(file_name(path))

errno = 0;
    int scanned = sscanf(
        file_name(path), "r.%d.%d.mcr", 
        region_file->region_x, 
        region_file->region_z
    );
    if (errno) return -1;

struct MC_RegionFile;
struct MC_RegionFile *MC_open_region_file(char *path);
void MC_close_region_file(struct MC_RegionFile);
