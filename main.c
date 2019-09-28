/*
	fbv  --  simple image viewer for the linux framebuffer
	Copyright (C) 2000, 2001, 2003, 2004  Mateusz 'mteg' Golicz

	fbsplash -- simple framebuffer splash display based on fbv
	lot of copy no rights
*/

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "config.h"
#include "fbv.h"

#define PAN_STEPPING 20

static int opt_clear = 1,
	   opt_alpha = 0,
	   opt_hide_cursor = 1,
	   opt_image_info = 1,
	   opt_stretch = 1,
	   opt_delay = 0,
	   opt_enlarge = 1,
	   opt_ignore_aspect = 1;

#define FREE_POINTER(x) free(x)

void setup_console(int t)
{
	struct termios our_termios;
	static struct termios old_termios;

	if(t)
	{
		tcgetattr(0, &old_termios);
		memcpy(&our_termios, &old_termios, sizeof(struct termios));
		our_termios.c_lflag &= !(ECHO | ICANON);
		tcsetattr(0, TCSANOW, &our_termios);
	}
	else
		tcsetattr(0, TCSANOW, &old_termios);

}

static inline void do_rotate(struct image *i, int rot)
{
	if(rot)
	{
		unsigned char *nextimage, *nextalpha=NULL;
		unsigned char *previmage, *prevalpha;
		int t;

		if (i->nextrgb)
			previmage = i->nextrgb;
		else
			previmage = i->rgb;
		if (i->nextalpha)
			prevalpha = i->nextalpha;
		else
			prevalpha = i->alpha;
		nextimage = rotate(previmage, i->width, i->height, rot);
		if(prevalpha)
			nextalpha = alpha_rotate(prevalpha, i->width, i->height, rot);

		if (i->nextrgb)
			FREE_POINTER(i->nextrgb);
		if (i->nextalpha)
			FREE_POINTER(i->nextalpha);
		i->nextrgb = nextimage;
		i->nextalpha = nextalpha;

		if(rot & 1)
		{
			t = i->width;
			i->width = i->height;
			i->height = t;
		}
	}
}


static inline void do_enlarge(struct image *i, int screen_width, int screen_height, int ignoreaspect)
{
	if(((i->width > screen_width) || (i->height > screen_height)) && (!ignoreaspect))
		return;
	if((i->width < screen_width) || (i->height < screen_height))
	{
		int xsize = i->width, ysize = i->height;
		unsigned char *nextimage, *nextalpha=NULL;
		unsigned char *previmage, *prevalpha;

		if(ignoreaspect)
		{
			if(i->width < screen_width)
				xsize = screen_width;
			if(i->height < screen_height)
				ysize = screen_height;

			goto have_sizes;
		}

		if((i->height * screen_width / i->width) <= screen_height)
		{
			xsize = screen_width;
			ysize = i->height * screen_width / i->width;
			goto have_sizes;
		}

		if((i->width * screen_height / i->height) <= screen_width)
		{
			xsize = i->width * screen_height / i->height;
			ysize = screen_height;
			goto have_sizes;
		}
		return;
have_sizes:
		if (i->nextrgb)
			previmage = i->nextrgb;
		else
			previmage = i->rgb;
		if (i->nextalpha)
			prevalpha = i->nextalpha;
		else
			prevalpha = i->alpha;
		nextimage = simple_resize(previmage, i->width, i->height, xsize, ysize);
		if(prevalpha)
			nextalpha = alpha_resize(prevalpha, i->width, i->height, xsize, ysize);

		i->width = xsize;
		i->height = ysize;
		if (i->nextrgb)
			FREE_POINTER(i->nextrgb);
		if (i->nextalpha)
			FREE_POINTER(i->nextalpha);
		i->nextrgb = nextimage;
		i->nextalpha = nextalpha;
	}
}


static inline void do_fit_to_screen(struct image *i, int screen_width, int screen_height, int ignoreaspect, int cal)
{
	if((i->width > screen_width) || (i->height > screen_height))
	{
		unsigned char *nextimage, *nextalpha=NULL;
		unsigned char *previmage, *prevalpha;
		int nx_size = i->width, ny_size = i->height;

		if(ignoreaspect)
		{
			if(i->width > screen_width)
				nx_size = screen_width;
			if(i->height > screen_height)
				ny_size = screen_height;
		}
		else
		{
			if((i->height * screen_width / i->width) <= screen_height)
			{
				nx_size = screen_width;
				ny_size = i->height * screen_width / i->width;
			}
			else
			{
				nx_size = i->width * screen_height / i->height;
				ny_size = screen_height;
			}
		}

		if (i->nextrgb)
			previmage = i->nextrgb;
		else
			previmage = i->rgb;
		if (i->nextalpha)
			prevalpha = i->nextalpha;
		else
			prevalpha = i->alpha;
		if(cal)
			nextimage = color_average_resize(previmage, i->width, i->height, nx_size, ny_size);
		else
			nextimage = simple_resize(previmage, i->width, i->height, nx_size, ny_size);

		if(prevalpha)
			nextalpha = alpha_resize(prevalpha, i->width, i->height, nx_size, ny_size);

		i->width = nx_size;
		i->height = ny_size;
		if (i->nextrgb)
			FREE_POINTER(i->nextrgb);
		if (i->nextalpha)
			FREE_POINTER(i->nextalpha);
		i->nextrgb = nextimage;
		i->nextalpha = nextalpha;
	}
}

static inline void do_display(struct image *i, int x_pan, int y_pan, int x_offs, int y_offs, int newimage)
{
	unsigned char *image, *alpha;

	if (i->nextrgb)
		image = i->nextrgb;
	else
		image = i->rgb;
	if (i->nextalpha)
		alpha = i->nextalpha;
	else
		alpha = i->alpha;

	if (newimage)
	{
		if (i->saved)
			FREE_POINTER(i->saved);
		i->saved = NULL;
	}

	fb_display(image, alpha, i->width, i->height, x_pan, y_pan, x_offs, y_offs,
					alpha ? &(i->saved) : NULL, newimage);

	if (i->nextrgb)
	{
		if (i->rgb)
			FREE_POINTER(i->rgb);
		i->rgb = i->nextrgb;
		i->nextrgb = NULL;
	}
	if (i->nextalpha)
	{
		if (i->alpha)
			FREE_POINTER(i->alpha);
		i->alpha = i->nextalpha;
		i->nextalpha = NULL;
	}
}

int show_image(char *filename)
{
	int (*load)(char *, unsigned char **, unsigned char **, int, int) = NULL;
	int (*loadnext)(unsigned char **, unsigned char **, int, int) = NULL;
	int (*refreshdelay)(void) = NULL;
	int (*unload)(void) = NULL;

	unsigned char * image_ptr = NULL;
	unsigned char * alpha_ptr = NULL;

	int x_size, y_size, screen_width, screen_height;
	int x_pan, y_pan, x_offs, y_offs, refresh = 1, c, ret = 1;
	int delay = opt_delay, retransform = 1;

	int transform_stretch = opt_stretch, transform_enlarge = opt_enlarge, transform_cal = (opt_stretch == 2),
	    transform_iaspect = opt_ignore_aspect, transform_rotation = 0;

	struct image i;

	struct timespec refresh_ts, starttime_ts, now_ts, delta_ts;
	struct timeval sleep_tv = { 0L , 1000L };
	int delta_ms;
	fd_set sleep_fds;

#ifdef FBV_SUPPORT_GIF
	if(fh_gif_id(filename))
		if(fh_gif_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
		{
			load = fh_gif_load;
			loadnext = fh_gif_next;
			refreshdelay = fh_gif_get_delay;
			unload = fh_gif_unload;
			goto identified;
		}
#endif

#ifdef FBV_SUPPORT_PNG
	if(fh_png_id(filename))
		if(fh_png_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
		{
			load = fh_png_load;
			goto identified;
		}
#endif

#ifdef FBV_SUPPORT_JPEG
	if(fh_jpeg_id(filename))
		if(fh_jpeg_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
		{
			load = fh_jpeg_load;
			goto identified;
		}
#endif

#ifdef FBV_SUPPORT_BMP
	if(fh_bmp_id(filename))
		if(fh_bmp_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
		{
			load = fh_bmp_load;
			goto identified;
		}
#endif
	fprintf(stderr, "%s: Unable to access file or file format unknown.\n", filename);
	return(1);

identified:

	if(load(filename, &image_ptr, opt_alpha ? &alpha_ptr : NULL, x_size, y_size) != FH_ERROR_OK)
	{
		fprintf(stderr, "%s: Image data is corrupt?\n", filename);
		goto error_mem;
	}

	clock_gettime(CLOCK_REALTIME, &starttime_ts);

	getCurrentRes(&screen_width, &screen_height);

	i.width = x_size;
	i.height = y_size;
	i.rgb = NULL;
	i.nextrgb = image_ptr;
	i.alpha = NULL;
	i.nextalpha = alpha_ptr;
	i.saved = NULL;

	while(1)
	{
		if(opt_clear) {
			printf("\033[H\033[J");
			fflush(stdout);
		}
		// 		do_rotate(&i, transform_rotation);
		do_enlarge(&i, screen_width, screen_height, transform_iaspect);
		do_fit_to_screen(&i, screen_width, screen_height, transform_iaspect, transform_cal);

		if(refresh)
		{
			if(i.width < screen_width)
				x_offs = (screen_width - i.width) / 2;
			else
				x_offs = 0;

			if(i.height < screen_height)
				y_offs = (screen_height - i.height) / 2;
			else
				y_offs = 0;

			do_display(&i, x_pan, y_pan, x_offs, y_offs, retransform);
			goto done;
		}
	}

done:
	if(opt_clear)
	{
		printf("\033[H\033[J");
		fflush(stdout);
	}

error_mem:
	if (unload)
		unload();
	if(i.nextrgb)
		FREE_POINTER(i.nextrgb);
	if(i.nextalpha)
		FREE_POINTER(i.nextalpha);
	if(i.rgb)
		FREE_POINTER(i.rgb);
	if(i.alpha)
		FREE_POINTER(i.alpha);
	if(i.saved)
		FREE_POINTER(i.saved);
	return(ret);
}

void sighandler(int s)
{
	if(opt_hide_cursor)
	{
		printf("\033[?25h");
		fflush(stdout);
	}
	setup_console(0);
	_exit(128 + s);
}

int main(int argc, char **argv)
{

	if(argc < 2)
	{
		printf("Provide at least a filename...\r\n");
		return(1);
	}

	signal(SIGHUP, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGSEGV, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGABRT, sighandler);

	if(opt_hide_cursor)
	{
		printf("\033[?25l");
		fflush(stdout);
	}

	setup_console(1);

	show_image(argv[1]);

	setup_console(0);

	if(opt_hide_cursor)
	{
		printf("\033[?25h");
		fflush(stdout);
	}
	return(0);
}
