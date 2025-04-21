char *file_name(const char *path);

struct file_buffer {
    char *data;
    int size;
    char *path;
};

struct file_buffer file_open(char *path);
void file_close(struct file_buffer);
