#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <clod/compression.h>

constexpr uint8_t data[] = {
#embed "fractal_Mandelbrot_3840x2160_644.png"
};

void compression_test(uint64_t algo) {
	constexpr size_t data_size = sizeof(data);

	struct clod_compressor *compressor = clod_compressor_init(nullptr, nullptr);
	check(compressor != nullptr);
	struct clod_decompressor *decompressor = clod_decompressor_init(nullptr, nullptr);
	check(decompressor != nullptr);

	size_t compressed_size;
	void *compressed = malloc(data_size);
	auto res = clod_compress(compressor,
		compressed, data_size,
		data, data_size,
		&compressed_size, algo, 5);
	check(res == CLOD_COMPRESSION_SUCCESS);
	check(compressed_size < data_size);

	res = clod_compress(compressor,
		compressed, data_size,
		data, data_size,
		nullptr, algo, 5);
	check(res == CLOD_COMPRESSION_SHORT_OUTPUT);

	res = clod_compress(compressor,
		compressed, compressed_size,
		data, data_size,
		nullptr, algo, 5);
	check(res == CLOD_COMPRESSION_SUCCESS);

	size_t decompressed_size;
	void *decompressed = malloc(data_size + 1);

	res = clod_decompress(decompressor,
		decompressed, data_size - 1,
		compressed, compressed_size,
		&decompressed_size, algo);
	check(res == CLOD_COMPRESSION_SHORT_BUFFER);
	check(decompressed_size == 0 || decompressed_size == data_size);

	res = clod_decompress(decompressor,
		decompressed, data_size - 1,
		compressed, compressed_size,
		nullptr, algo);
	check(res == CLOD_COMPRESSION_SHORT_BUFFER);

	res = clod_decompress(decompressor,
		decompressed, data_size + 1,
		compressed, compressed_size,
		nullptr, algo);
	check(res == CLOD_COMPRESSION_SHORT_OUTPUT);

	res = clod_decompress(decompressor,
		decompressed, data_size,
		compressed, compressed_size,
		&decompressed_size, algo);
	check(res == CLOD_COMPRESSION_SUCCESS);
	check(decompressed_size == data_size);
	check(memcmp(data, decompressed, data_size) == 0);

	res = clod_decompress(decompressor,
		decompressed, data_size,
		compressed, compressed_size,
		nullptr, algo);
	check(res == CLOD_COMPRESSION_SUCCESS);

	free(compressed);
	free(decompressed);
	clod_compressor_free(compressor);
	clod_decompressor_free(decompressor);
}
