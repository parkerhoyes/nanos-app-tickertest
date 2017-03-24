/*
 * License for the Ledger Nano S Ticker Test Application project, originally found
 * here: https://github.com/parkerhoyes/nanos-app-tickertest
 *
 * Copyright (C) 2017 Parker Hoyes <contact@parkerhoyes.com>
 *
 * This software is provided "as-is", without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose, including
 * commercial applications, and to alter it and redistribute it freely, subject to
 * the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim
 *    that you wrote the original software. If you use this software in a product,
 *    an acknowledgment in the product documentation would be appreciated but is
 *    not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "app.h"

#include <stdbool.h>
#include <stdint.h>

#include "os.h"
#include "os_io_seproxyhal.h"

#include "bui.h"
#include "bui_font.h"

#define APP_TICKER_INTERVAL 25

static bui_ctx_t app_bui_ctx;
static bool app_seproxy_ready;
static bool app_disp_invalidated;

// The time elapsed since the app started in milliseconds
static uint32_t app_time_elapsed;

static uint8_t app_udec_encode(uint32_t i, char *dest) {
	if (i == 0) {
		*dest = '0';
		return 1;
	}
	uint8_t len = 0;
	for (uint32_t n = i; n != 0; n /= 10)
		len += 1;
	dest += len;
	for (uint8_t j = len; j != 0; j--) {
		*--dest = '0' + i % 10;
		i /= 10;
	}
	return len;
}

static void app_draw() {
	bui_font_draw_string(&app_bui_ctx, "Time Elapsed:", 64, 4, BUI_DIR_TOP, bui_font_open_sans_regular_11);
	{
		char text[13];
		uint8_t len = app_udec_encode(app_time_elapsed, text);
		text[len++] = 'm';
		text[len++] = 's';
		text[len] = '\0';
		bui_font_draw_string(&app_bui_ctx, text, 64, 16, BUI_DIR_TOP, bui_font_open_sans_regular_11);
	}
}

static void app_tick() {
	app_draw();
	app_time_elapsed += APP_TICKER_INTERVAL;
	app_disp_invalidated = true;
}

static void app_display() {
	bui_ctx_fill(&app_bui_ctx, false);
	app_draw();
	if (app_seproxy_ready)
		app_seproxy_ready = !bui_ctx_display(&app_bui_ctx);
}

void app_init() {
	// Set a ticker interval of APP_TICKER_INTERVAL ms
	G_io_seproxyhal_spi_buffer[0] = SEPROXYHAL_TAG_SET_TICKER_INTERVAL;
	G_io_seproxyhal_spi_buffer[1] = 0;
	G_io_seproxyhal_spi_buffer[2] = 2;
	G_io_seproxyhal_spi_buffer[3] = 0;
	G_io_seproxyhal_spi_buffer[4] = APP_TICKER_INTERVAL;
	io_seproxyhal_spi_send(G_io_seproxyhal_spi_buffer, 5);

	// Initialize global vars
	bui_ctx_init(&app_bui_ctx);
	app_seproxy_ready = true;
	app_disp_invalidated = true;
	app_time_elapsed = 0;

	// First tick
	app_tick();
}

void app_event_button_push(unsigned int button_mask, unsigned int button_mask_counter) {
	switch (button_mask) {
	case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
		os_sched_exit(0); // Return to dashboard
		break;
	case BUTTON_EVT_RELEASED | BUTTON_LEFT:
		// Do nothing
		break;
	case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
		// Do nothing
		break;
	}
}

void app_event_ticker() {
	app_tick();
	if (app_disp_invalidated) {
		app_display();
		app_disp_invalidated = false;
	}
}

void app_event_display_processed() {
	app_seproxy_ready = !bui_ctx_display(&app_bui_ctx);
}
