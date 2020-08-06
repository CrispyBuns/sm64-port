#ifdef TARGET_DOS

#include "macros.h"

#include "gfx_dos_api.h"

#include "gfx_opengl.h"

#include <dos.h>

#include <stdio.h>


#define VIDEO_INT           0x10      /* the BIOS video interrupt. */
#define WRITE_DOT           0x0C      /* BIOS func to plot a pixel. */
#define SET_MODE            0x00      /* BIOS func to set the video mode. */
#define VGA_256_COLOR_MODE  0x13      /* use to set 256-color mode. */
#define TEXT_MODE           0x03      /* use to set 80x25 text mode. */

#define SCREEN_WIDTH        320       /* width in pixels of mode 0x13 */
#define SCREEN_HEIGHT       200       /* height in pixels of mode 0x13 */
#define NUM_COLORS          256       /* number of colors in mode 0x13 */

typedef unsigned char  byte;
byte *VGA = (byte *)0xA0000;          /* this points to video memory. */

void *osmesa_buffer;          // 320x200x4 (RGBA)
unsigned char *screen_buffer; // 320x200x1 (VGA_256)

void set_mode_13()
{
    union REGS regs;
    regs.h.ah = 0x00;  // function 00h = mode set
    regs.h.al = VGA_256_COLOR_MODE; //
    int86(0x10, &regs, &regs);
}

#include <sys/nearptr.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
static void gfx_dos_init(UNUSED const char *game_name, UNUSED bool start_in_fullscreen)
{
    printf("gfx_dos_init\n");

    if (__djgpp_nearptr_enable() == 0)
    {
      printf("Could get access to first 640K of memory.\n");
      abort();
    }

    VGA+=__djgpp_conventional_base;

    screen_buffer = (unsigned char *) malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(unsigned char));

    set_mode_13();
}

static void gfx_dos_set_keyboard_callbacks(UNUSED bool (*on_key_down)(int scancode), UNUSED bool (*on_key_up)(int scancode), UNUSED void (*on_all_keys_up)(void))
{
}

static void gfx_dos_set_fullscreen_changed_callback(UNUSED void (*on_fullscreen_changed)(bool is_now_fullscreen))
{
}

static void gfx_dos_set_fullscreen(UNUSED bool enable)
{
}

static void gfx_dos_main_loop(void (*run_one_game_iter)(void))
{
    run_one_game_iter();
}

static void gfx_dos_get_dimensions(uint32_t *width, uint32_t *height)
{
    *width = SCREEN_WIDTH;
    *height = SCREEN_HEIGHT;
}

static void gfx_dos_handle_events(void)
{
}

static bool gfx_dos_start_frame(void)
{
    return true;
}

int colour_find_rgb(byte r, byte g, byte b);


void draw_to_screen(const unsigned char* inbuf, int width, int height)
{
    for (int h = 0; h < height; h++)
    {
        for (int w = 0; w < width; w++)
        {
            int i = (h*width + w) * 4;

            byte r = *(inbuf + i + 0);
            byte g = *(inbuf + i + 1);
            byte b = *(inbuf + i + 2);
            // byte a = *(inbuf + i + 3);
            ((char *)screen_buffer)[h*width + w] = colour_find_rgb(r, g, b);
        }
    }
    memcpy(VGA, screen_buffer, width*height);
}

static void gfx_dos_swap_buffers_begin(void)
{
    if (osmesa_buffer != NULL) {
        draw_to_screen(osmesa_buffer, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

static void gfx_dos_swap_buffers_end(void)
{
}

static double gfx_dos_get_time(void)
{
    return 0.0;
}

struct GfxWindowManagerAPI gfx_dos_api =
{
    gfx_dos_init,
    gfx_dos_set_keyboard_callbacks,
    gfx_dos_set_fullscreen_changed_callback,
    gfx_dos_set_fullscreen,
    gfx_dos_main_loop,
    gfx_dos_get_dimensions,
    gfx_dos_handle_events,
    gfx_dos_start_frame,
    gfx_dos_swap_buffers_begin,
    gfx_dos_swap_buffers_end,
    gfx_dos_get_time
};



static int
colour_dist_sq(int R, int G, int B, int r, int g, int b)
{
	return ((R - r) * (R - r) + (G - g) * (G - g) + (B - b) * (B - b));
}

static int
colour_to_6cube(int v)
{
	if (v < 48)
		return (0);
	if (v < 114)
		return (1);
	return ((v - 35) / 40);
}

/*
 * Convert an RGB triplet to the xterm(1) 256 colour palette.
 *
 * xterm provides a 6x6x6 colour cube (16 - 231) and 24 greys (232 - 255). We
 * map our RGB colour to the closest in the cube, also work out the closest
 * grey, and use the nearest of the two.
 *
 * Note that the xterm has much lower resolution for darker colours (they are
 * not evenly spread out), so our 6 levels are not evenly spread: 0x0, 0x5f
 * (95), 0x87 (135), 0xaf (175), 0xd7 (215) and 0xff (255). Greys are more
 * evenly spread (8, 18, 28 ... 238).
 */
int
colour_find_rgb(byte r, byte g, byte b)
{
	static const int	q2c[6] = { 0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff };
	int			qr, qg, qb, cr, cg, cb, d, idx;
	int			grey_avg, grey_idx, grey;

	/* Map RGB to 6x6x6 cube. */
	qr = colour_to_6cube(r); cr = q2c[qr];
	qg = colour_to_6cube(g); cg = q2c[qg];
	qb = colour_to_6cube(b); cb = q2c[qb];

	/* If we have hit the colour exactly, return early. */
	if (cr == r && cg == g && cb == b)
	   return ((16 + (36 * qr) + (6 * qg) + qb));

	/* Work out the closest grey (average of RGB). */
	grey_avg = (r + g + b) / 3;
	if (grey_avg > 238)
		grey_idx = 23;
	else
		grey_idx = (grey_avg - 3) / 10;
	grey = 8 + (10 * grey_idx);

	/* Is grey or 6x6x6 colour closest? */
	d = colour_dist_sq(cr, cg, cb, r, g, b);
	if (colour_dist_sq(grey, grey, grey, r, g, b) < d)
		idx = 232 + grey_idx;
	else
		idx = 16 + (36 * qr) + (6 * qg) + qb;
	return (idx);
}

#endif
