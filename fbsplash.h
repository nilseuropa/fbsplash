/*
	fbv  --  simple image viewer for the linux framebuffer
	Copyright (C) 2000, 2001, 2003, 2004  Mateusz 'mteg' Golicz

	fbsplash -- simple framebuffer splash display based on fbv
	lot of copy no rights
*/


#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1		/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */
#define FH_ERROR_MEM 3		/* memory alloc error */

void fb_display(unsigned char *rgbbuff, unsigned char * alpha, int x_size, int y_size, int x_pan, int y_pan, int x_offs, int y_offs, unsigned char **savebuf, int save);
void getCurrentRes(int *x, int *y);

int fh_bmp_id(char *name);
int fh_bmp_load(char *name,unsigned char **buffer, unsigned char **alpha, int x,int y);
int fh_bmp_unload(void);
int fh_bmp_getsize(char *name,int *x,int *y);

int fh_jpeg_id(char *name);
int fh_jpeg_load(char *name,unsigned char **buffer, unsigned char **alpha, int x,int y);
int fh_jpeg_unload(void);
int fh_jpeg_getsize(char *name,int *x,int *y);

int fh_png_id(char *name);
int fh_png_load(char *name,unsigned char **buffer, unsigned char **alpha, int x,int y);
int fh_png_unload(void);
int fh_png_getsize(char *name,int *x,int *y);

int fh_gif_id(char *name);
int fh_gif_load(char *name,unsigned char **buffer, unsigned char **alpha, int x,int y);
int fh_gif_next(unsigned char **buffer, unsigned char **alpha, int x, int y);
int fh_gif_unload(void);
int fh_gif_getsize(char *name,int *x,int *y);
int fh_gif_get_delay(void);
int fh_gif_get_disposal_method(void);
int fh_gif_get_userinput(void);

struct image
{
	int width, height;
	unsigned char *rgb;
	unsigned char *alpha;
	unsigned char *nextrgb;
	unsigned char *nextalpha;
	unsigned char *saved;
};

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
unsigned char * simple_resize(unsigned char * orgin,int ox,int oy,int dx,int dy);
unsigned char * alpha_resize(unsigned char * alpha,int ox,int oy,int dx,int dy);
unsigned char * color_average_resize(unsigned char * orgin,int ox,int oy,int dx,int dy);
unsigned char * rotate(unsigned char *i, int ox, int oy, int rot);
unsigned char * alpha_rotate(unsigned char *i, int ox, int oy, int rot);

#ifdef DEBUG
	extern int debugme;
	#define BUG if (debugme) { fprintf(stderr, "BUG: %s line=%d\n",__FILE__, __LINE__); }
#else
	#define debugme 0
	#define BUG
#endif
