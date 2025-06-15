#include "../platform.h"
#include <sys/sysinfo.h>

int num_procs() {
	return get_nprocs();
}
