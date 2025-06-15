#include <clod/region.h>
#include "region_impl.h"
#include "region_file.h"
#include "error.h"

enum clod_region_result clod_region_read(
	struct clod_region *region,
	const int64_t *pos,
	uint8_t *,
	size_t,
	size_t *
) {
	REGION_PUBLIC_ENTER(region);

	mutex_lock(&region->mtx);

	struct region_file *rf;
	auto res = region_file_get(region, &rf, pos, false);
	if (res != CLOD_REGION_OK) {
		mutex_unlock(&region->mtx);
		REGION_PUBLIC_LEAVE(region);
		return res;
	}

	rwmutex_rdlock(&rf->mtx);
	mutex_unlock(&region->mtx);

	uint8_t *data;
	size_t size;
	res = file_get(rf->f, (void**)&data, &size);
	if (res != CLOD_REGION_OK) {
		rwmutex_rdunlock(&rf->mtx);
		REGION_PUBLIC_LEAVE(region);
		return res;
	}

	REGION_PUBLIC_LEAVE(region);
	return res;
}
