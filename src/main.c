/*
 * YD-RP2040 + OV7670 minimal capture test.
 *
 * Captures one 80x60 YUV422 frame from the OV7670 and prints it as ASCII art
 * over USB serial, just to confirm the wiring/PIO capture path works before
 * building anything more elaborate on top.
 *
 * Camera library: https://github.com/usedbytes/camera-pico-ov7670
 */

#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "camera/camera.h"
#include "camera/format.h"

#define CAMERA_PIO      pio0

// Parallel bus pins (74HC165 side), consecutive from CAMERA_BASE_PIN:
//   CAMERA_BASE_PIN + 0 = Qh (serial data out of 74HC165)
//   CAMERA_BASE_PIN + 1 = 74HC165 CLK
//   CAMERA_BASE_PIN + 2 = OV7670 VSYNC
//   CAMERA_BASE_PIN + 3 = OV7670 HREF
//   CAMERA_BASE_PIN + 4 = OV7670 PCLK (also drives 74HC165 SH/nLD)
#define CAMERA_BASE_PIN 6

// Must be one of the GPOUT clock pins: 21, 23, 24 or 25
#define CAMERA_XCLK_PIN 21

#define CAMERA_SDA      4
#define CAMERA_SCL      5

static inline int __i2c_write_blocking(void *i2c_handle, uint8_t addr, const uint8_t *src, size_t len)
{
	return i2c_write_blocking((i2c_inst_t *)i2c_handle, addr, src, len, false);
}

static inline int __i2c_read_blocking(void *i2c_handle, uint8_t addr, uint8_t *dst, size_t len)
{
	return i2c_read_blocking((i2c_inst_t *)i2c_handle, addr, dst, len, false);
}

// From http://www.paulbourke.net/dataformats/asciiart/
static const char charmap[] = " .:-=+*#%@";

int main() {
	stdio_init_all();

	// Wait for a USB serial connection so the first frames aren't lost
	sleep_ms(3000);

	const uint LED_PIN = PICO_DEFAULT_LED_PIN;
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	i2c_init(i2c0, 100000);
	gpio_set_function(CAMERA_SDA, GPIO_FUNC_I2C);
	gpio_set_function(CAMERA_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(CAMERA_SDA);
	gpio_pull_up(CAMERA_SCL);

	struct camera camera;
	struct camera_platform_config platform = {
		.i2c_write_blocking = __i2c_write_blocking,
		.i2c_read_blocking = __i2c_read_blocking,
		.i2c_handle = i2c0,

		.pio = CAMERA_PIO,
		.xclk_pin = CAMERA_XCLK_PIN,
		.xclk_divider = 9,
		.base_pin = CAMERA_BASE_PIN,
		.base_dma_channel = -1,
	};

	printf("Initialising OV7670...\n");
	int ret = camera_init(&camera, &platform);
	if (ret) {
		printf("camera_init failed: %d\n", ret);
		printf("-> Cek wiring I2C (SIOC/SIOD), XCLK, dan power 3.3V ke kamera.\n");
		while (1) {
			gpio_put(LED_PIN, 1);
			sleep_ms(100);
			gpio_put(LED_PIN, 0);
			sleep_ms(100);
		}
	}
	printf("OV7670 detected OK\n");

	const uint16_t width = CAMERA_WIDTH_DIV8;   // 80
	const uint16_t height = CAMERA_HEIGHT_DIV8; // 60

	struct camera_buffer *buf = camera_buffer_alloc(FORMAT_YUV422, width, height);
	assert(buf);

	while (1) {
		printf("Capturing...\n");
		gpio_put(LED_PIN, 1);
		ret = camera_capture_blocking(&camera, buf, true);
		gpio_put(LED_PIN, 0);

		if (ret != 0) {
			printf("Capture error: %d\n", ret);
		} else {
			printf("Capture success\n");
			for (int y = 0; y < height; y++) {
				char row[width + 1];
				for (int x = 0; x < width; x++) {
					uint8_t v = buf->data[0][buf->strides[0] * y + x];
					v /= 26;
					row[x] = charmap[v];
				}
				row[width] = '\0';
				printf("%s\n", row);
			}
		}

		sleep_ms(3000);
	}
}
