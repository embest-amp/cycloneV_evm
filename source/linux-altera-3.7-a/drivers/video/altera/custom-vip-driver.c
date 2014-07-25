#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "my_app.h"
#include "system.h"

#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>

#include <linux/fb.h>

//#include "720x576x4.h"
//#include "image_file.h"


#define FRAME1_BUF_W 		ALT_VIDEO_DISPLAY_COLS
#define FRAME1_BUF_H 		ALT_VIDEO_DISPLAY_ROWS

#define BG_FRAME_W 		FRAME1_BUF_W
#define BG_FRAME_H			FRAME1_BUF_H

#define ALTERA_MMAP_MODE0
//#define ALTERA_MMAP_MODE1

#define MODE_STRING						"1920x1080-32@60"

#define VIDEOMEMSIZE 					(0x2000000)
#define FBSTARTADDR	    					(0xC0000000)

#define VFR0_SCREEN_0_BASE_ADDRESS ((int)0x0)
//#define VFR0_SCREEN_0_BASE_ADDRESS ((int)0x10000000)
#define VFR0_SCREEN_PIXEL_COUNT  ( FRAME1_BUF_W * FRAME1_BUF_H)

static int __init vfb_init(void);
static ssize_t altera_lcd_fbmode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t altera_lcd_fbmode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
static u_long get_line_length(int xres_virtual, int bpp);

static int xres = 0, yres = 0, refresh = 60, bpp = 32;

static struct device_attribute altera_lcd_class_attrs[] = 
{
	__ATTR(fbmode, 0644, altera_lcd_fbmode_show, altera_lcd_fbmode_store),
	__ATTR_NULL,
};



typedef  struct
{
	u32 packet_go;		//0x0
	u32 status;
	u32 interrupt;
	u32 packet_bank;
	u32 pb0_base;
	u32 pb0_words;
	u32 pb0_samples;
	u32 res1;
	u32 pb0_width;
	u32 pb0_height;
	u32 pb0_interlaced; //0x11
}lcd_ctrl_reg;


typedef struct
{
	lcd_ctrl_reg 		*fpga_ctlr_reg_virt_base;
	u32				 fpga_ctrl_reg_phy_base;
	u32			 	fpga_ctrl_reg_len;

	u32			 	fpga_frame_0_phy;
	
	u32			 	mpu_frame_0_phy;
	u32			 	*mpu_frame_0_virt;
	
	u32				fpga_mpu_frame_len;
	
	u32				xres;
	u32				yres;
	u32				bpp;
	u32				refresh;

	u32				lcd_mode_default;
	char				mode[32];
	
}fpga_lcd_info;



#if 0
static void display_picture()
{
	

}
#endif

static int altera_lcd_parse_mode(const char *str,u32 *xres, u32 *yres, u32 *bpp, u32 *refresh)
{
	 u32 len;
	 int i;

	if(str)
	{
		len = strlen(str);
		if(len <= 0)
		{
			return -1;
		}
	}
	else
		return -1;
	
	for(i = len - 1; i>=0; i--)
	{
		switch(str[i])
		{
			case 'x':
			case 'X':
				*yres = simple_strtol(str + i + 1, NULL, 10);
				break;
			case '-':
				*bpp = simple_strtol(str + i + 1, NULL, 10);
				break;
			case '@':
				*refresh = simple_strtol(str + i + 1, NULL, 10);
				break;
			case '0' ... '9':
				break;
			default:
				break;
			
		}
	
	}

	if((i < 0) && (*yres))
	{
		*xres = simple_strtol(str, NULL, 10);
	}
	else
		return -1;

	if((*bpp == 0)||(*xres == 0)||(*yres == 0))
		return -1;

	return 0;

}

static ssize_t altera_lcd_fbmode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct fb_info 	*info		= dev_get_drvdata(dev);
	fpga_lcd_info   	* lcd_info	= (fpga_lcd_info   *)info->par;

	return snprintf(buf, 32, "%s\n", lcd_info->mode);
}

static ssize_t altera_lcd_fbmode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct fb_info			*info 		= dev_get_drvdata(dev);
	fpga_lcd_info				*lcd_info	= (fpga_lcd_info *)info->par;
	struct fb_var_screeninfo 	*var		= &info->var;
	ssize_t					ret 			= -EINVAL;
	u32						len;
	u32						xres = 0,yres = 0,bpp = 0,refresh = 0;
	u32						pixel_cnt;

	if (!lock_fb_info(info))
			return -ENODEV;

	len = (size >= 32) ? 32 : size;
	//snprintf(lcd_info->mode, len,"%s", buf);

	ret = altera_lcd_parse_mode(buf, &xres, &yres, &bpp, &refresh);
	if(ret < 0)
	{
		printk("altera lcd store fbmode failed\n");
		unlock_fb_info(info);
		return ret;
	}
	else
	{
		printk("switch to new mode:xres=%d,yres=%d,bpp=%d,refresh=%d\n",xres,yres,bpp,refresh);
	}

	var->xres 			= xres;
	var->yres 			= yres;
	var->xres_virtual 		= xres;
	var->yres_virtual 		= yres;
	var->bits_per_pixel	=  bpp;

	if (info->fbops->fb_check_var)
	{
		ret = info->fbops->fb_check_var(var, info);
		if(ret < 0)
		{
			printk("new mode does not pass check var!restore to old var!\n");
			/* restore our old parameter */
			var->xres 			= lcd_info->xres;
			var->yres 			= lcd_info->yres;
			var->xres_virtual 		= lcd_info->xres;
			var->yres_virtual 		= lcd_info->yres;
			var->bits_per_pixel	= lcd_info->bpp;
			unlock_fb_info(info);
			return ret;
		}
	}
	/* only when check is success,and we can update */
	snprintf(lcd_info->mode, len,"%s", buf);
	lcd_info->xres					= xres;
	lcd_info->yres				= yres;
	lcd_info->bpp					= bpp;
	lcd_info->refresh				= refresh;
	

	pixel_cnt = lcd_info->xres * lcd_info->yres;

	writel(pixel_cnt , &lcd_info->fpga_ctlr_reg_virt_base->pb0_words);
	writel(pixel_cnt , &lcd_info->fpga_ctlr_reg_virt_base->pb0_samples);
	writel(lcd_info->xres, &lcd_info->fpga_ctlr_reg_virt_base->pb0_width);
	writel(lcd_info->yres , &lcd_info->fpga_ctlr_reg_virt_base->pb0_height);

	info->fix.line_length = get_line_length(info->var.xres_virtual, info->var.bits_per_pixel);//update line length

	printk("line_length = %d,pb0_words=0x%x,pb0_samples=0x%x,pb0_width=%d,pb0_height=%d\n", info->fix.line_length, readl(&lcd_info->fpga_ctlr_reg_virt_base->pb0_words),
		                                                                                                                                                          readl(&lcd_info->fpga_ctlr_reg_virt_base->pb0_samples) ,	
		                                                                                                                                                          readl(&lcd_info->fpga_ctlr_reg_virt_base->pb0_width),
		                                                                                                                                                          readl(&lcd_info->fpga_ctlr_reg_virt_base->pb0_height)
		                                                                                                                                                          );

	unlock_fb_info(info);

	return len;
}


static int __init fpga_controller_init(struct fb_info *info)
{
	fpga_lcd_info *lcd_info = (fpga_lcd_info *)info->par;
	int			ret = -1;
	int			pixel_cnt, lcd_width, lcd_height,lcd_bpp,lcd_mode_default = 1,lcd_refresh;

	u32			i;
	u32			*video_mem_32;

	if((!xres) || (!yres) ||(!bpp))
	{
			pixel_cnt 		= VFR0_SCREEN_PIXEL_COUNT;
			lcd_width  		= FRAME1_BUF_W;
			lcd_height 		= FRAME1_BUF_H;
			lcd_bpp			= ALT_VIDEO_DISPLAY_COLOR_DEPTH;
			lcd_refresh		= 60;
			lcd_mode_default	= 1;
	}
	else
	{
			pixel_cnt 		= xres*yres;
			lcd_width 		= xres;
			lcd_height 		= yres;
			lcd_bpp			= bpp;
			lcd_refresh		= refresh;
			lcd_mode_default	= 0;
	}

	lcd_info->refresh 					= lcd_refresh;
	lcd_info->bpp						= lcd_bpp;
	lcd_info->xres						= lcd_width;
	lcd_info->yres					= lcd_height;
	lcd_info->lcd_mode_default			= lcd_mode_default;

	lcd_info->fpga_ctrl_reg_phy_base 	= ALT_VIP_VFR_0_BASE;
	lcd_info->fpga_ctrl_reg_len			= ALT_VIP_VFR_0_SPAN;
	lcd_info->fpga_ctlr_reg_virt_base 	= ioremap(lcd_info->fpga_ctrl_reg_phy_base, lcd_info->fpga_ctrl_reg_len);
	if(!lcd_info->fpga_ctlr_reg_virt_base)
	{
		printk("ioremap fpga lcd controller register failed\n");
		ret =  -ENOMEM;
		goto err;
	}

	lcd_info->fpga_frame_0_phy			= VFR0_SCREEN_0_BASE_ADDRESS&0x07ffffff;
	lcd_info->mpu_frame_0_phy			= FBSTARTADDR + VFR0_SCREEN_0_BASE_ADDRESS;
	lcd_info->fpga_mpu_frame_len		= VIDEOMEMSIZE;
	lcd_info->mpu_frame_0_virt			= ioremap(lcd_info->mpu_frame_0_phy, lcd_info->fpga_mpu_frame_len);
	if(!lcd_info->mpu_frame_0_virt)
	{
		printk("ioremap picture frame address for mpu failed\n");
		ret =  -ENOMEM;
		goto err1;
	}

	
	memset(lcd_info->mpu_frame_0_virt, 0x0, lcd_info->fpga_mpu_frame_len);

	video_mem_32 = lcd_info->mpu_frame_0_virt;
#if 0
	#if 1
	for(i = 0; i < lcd_info->xres*lcd_info->yres*(lcd_info->bpp>>3); i += 4)
	{
		*video_mem_32 = gImage_720x576[i]|(gImage_720x576[i+1]<<8)|(gImage_720x576[i+2]<<16)|(gImage_720x576[i+3]<<24);
		video_mem_32++;
	}
	#else
	for(i = 0; i < lcd_info->xres*lcd_info->yres*(lcd_info->bpp>>3); i += 4)
	{
		*video_mem_32 = g_imag[i]|(g_imag[i+1]<<8)|(g_imag[i+2]<<16)|(g_imag[i+3]<<24);
		video_mem_32++;
	}
	#endif
#endif

	writel(lcd_info->fpga_frame_0_phy, &lcd_info->fpga_ctlr_reg_virt_base->pb0_base);
	writel(pixel_cnt , &lcd_info->fpga_ctlr_reg_virt_base->pb0_words);
	writel(pixel_cnt , &lcd_info->fpga_ctlr_reg_virt_base->pb0_samples);
	writel(lcd_info->xres, &lcd_info->fpga_ctlr_reg_virt_base->pb0_width);
	writel(lcd_info->yres , &lcd_info->fpga_ctlr_reg_virt_base->pb0_height);
	writel(3 , &lcd_info->fpga_ctlr_reg_virt_base->pb0_interlaced);
	writel(0 , &lcd_info->fpga_ctlr_reg_virt_base->packet_bank);//switch to bank0
	writel(1 , &lcd_info->fpga_ctlr_reg_virt_base->packet_go);// Start Frame Reader 0

	return 0;
err1:
	iounmap(lcd_info->fpga_ctlr_reg_virt_base);
err:
	return ret;
}


static int __init custom_vip_driver_init(void)
{
        printk(KERN_ALERT "Registering Frame Buffer Driver\n");
        vfb_init();
       
        printk(KERN_ALERT "Done initialization\n");

	return 0;
}

static void __exit custom_vip_driver_exit(void)
{

	printk(KERN_ALERT "vip Driver is unloaded!\n");
}

    /*
     *  RAM we reserve for the frame buffer. This defines the maximum screen
     *  size
     *
     *  The default can be overridden if the driver is compiled as a module
     */

static struct fb_var_screeninfo vfb_default __devinitdata = {
	.xres =		BG_FRAME_W,
	.yres =		BG_FRAME_H,
	.xres_virtual =	BG_FRAME_W,
	.yres_virtual =	BG_FRAME_H,
	.bits_per_pixel = ALT_VIDEO_DISPLAY_COLOR_DEPTH,
	.red =		{ 0, 8, 0 },
      	.green =	{ 0, 8, 0 },
      	.blue =		{ 0, 8, 0 },
      	.activate =	FB_ACTIVATE_TEST,
      	.height =	BG_FRAME_H,
      	.width =	BG_FRAME_W,
      	.pixclock =	20000,
      	.left_margin =	64,
      	.right_margin =	64,
      	.upper_margin =	32,
      	.lower_margin =	32,
      	.hsync_len =	64,
      	.vsync_len =	2,
      	.vmode =	FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo vfb_fix __devinitdata = {
	.id =		"Altera LCD FB",
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =	FB_VISUAL_PSEUDOCOLOR,
	.xpanstep =	1,
	.ypanstep =	1,
	.ywrapstep =	1,
	.accel =	FB_ACCEL_NONE,
};

static int vfb_check_var(struct fb_var_screeninfo *var,
			 struct fb_info *info);
static int vfb_set_par(struct fb_info *info);
static int vfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			 u_int transp, struct fb_info *info);
static int vfb_pan_display(struct fb_var_screeninfo *var,
			   struct fb_info *info);
static int vfb_mmap(struct fb_info *info,
		    struct vm_area_struct *vma);

static struct fb_ops vfb_ops = {
	.fb_read        = fb_sys_read,
	.fb_write       = fb_sys_write,
	.fb_check_var	= vfb_check_var,
	.fb_set_par	= vfb_set_par,
	.fb_setcolreg	= vfb_setcolreg,
	.fb_pan_display	= vfb_pan_display,
	.fb_fillrect	= sys_fillrect,
	.fb_copyarea	= sys_copyarea,
	.fb_imageblit	= sys_imageblit,
	.fb_mmap	= vfb_mmap,
};

    /*
     *  Internal routines
     */

static u_long get_line_length(int xres_virtual, int bpp)
{
	u_long length;

	length = xres_virtual * bpp;
	length = (length + 31) & ~31;
	length >>= 3;
	return (length);
}

    /*
     *  Setting the video mode has been split into two parts.
     *  First part, xxxfb_check_var, must not write anything
     *  to hardware, it should only verify and adjust var.
     *  This means it doesn't alter par but it does use hardware
     *  data from it to check this var. 
     */

static int vfb_check_var(struct fb_var_screeninfo *var,
			 struct fb_info *info)
{
	u_long line_length;
	fpga_lcd_info *lcd_info = (fpga_lcd_info *)info->par;
	
	/*
	 *  FB_VMODE_CONUPDATE and FB_VMODE_SMOOTH_XPAN are equal!
	 *  as FB_VMODE_SMOOTH_XPAN is only used internally
	 */

	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = info->var.xoffset;
		var->yoffset = info->var.yoffset;
	}

	/*
	 *  Some very basic checks
	 */
	if (!var->xres)
		var->xres = 1;
	if (!var->yres)
		var->yres = 1;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	#if 1
	if (var->yres >= var->yres_virtual)
	{
		//printk("info->fix.ypanstep=%d,info->fix.xpanstep=%d\n", info->fix.ypanstep, info->fix.ypanstep);
		var->yres_virtual = var->yres*(info->fix.ypanstep + 1);//xuewt modify for double frame buffer
	}
	#else
	if (var->yres >= var->yres_virtual)
	{
		printk("info->fix.ypanstep=%d,info->fix.xpanstep=%d\n", info->fix.ypanstep, info->fix.ypanstep);
		var->yres_virtual = var->yres;//xuewt modify for double frame buffer
	}
	#endif
	
	if (var->bits_per_pixel <= 1)
		var->bits_per_pixel = 1;
	else if (var->bits_per_pixel <= 8)
		var->bits_per_pixel = 8;
	else if (var->bits_per_pixel <= 16)
		var->bits_per_pixel = 16;
	else if (var->bits_per_pixel <= 24)
		var->bits_per_pixel = 24;
	else if (var->bits_per_pixel <= 32)
		var->bits_per_pixel = 32;
	else
	{
		printk("Invalid parameter!\n");
		return -EINVAL;
	}

	if (var->xres_virtual < var->xoffset + var->xres)
		var->xres_virtual = var->xoffset + var->xres;
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	/*
	 *  Memory limit
	 */
	 #if 1
	 if(!info->fix.line_length)
	 {
	 		info->fix.line_length = get_line_length(var->xres_virtual, var->bits_per_pixel);
	 		//printk("line_length = %d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", info->fix.line_length);
	 }
	 #endif
	 
	line_length =
	    get_line_length(var->xres_virtual, var->bits_per_pixel);
	if (line_length * var->yres_virtual > lcd_info->fpga_mpu_frame_len)
	{
		printk("No memory in %s\n",__func__);
		return -ENOMEM;
	}

	/*
	 * Now that we checked it we alter var. The reason being is that the video
	 * mode passed in might not work but slight changes to it might make it 
	 * work. This way we let the user know what is acceptable.
	 */
	switch (var->bits_per_pixel) {
	case 1:
	case 8:
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 0;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 16:		/* RGBA 5551 */
		if (var->transp.length) {
			var->red.offset = 0;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 5;
			var->blue.offset = 10;
			var->blue.length = 5;
			var->transp.offset = 15;
			var->transp.length = 1;
		} else {	/* RGB 565 */
			var->red.offset = 0;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 6;
			var->blue.offset = 11;
			var->blue.length = 5;
			var->transp.offset = 0;
			var->transp.length = 0;
		}
		break;
	case 24:		/* RGB 888 */
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 16;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 32:		/* RGBA 8888 */
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 16;
		var->blue.length = 8;
		var->transp.offset = 24;
		var->transp.length = 8;
		break;
	}
	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;

	return 0;
}

/* This routine actually sets the video mode. It's in here where we
 * the hardware state info->par and fix which can be affected by the 
 * change in par. For this driver it doesn't do much. 
 */
static int vfb_set_par(struct fb_info *info)
{
	info->fix.line_length = get_line_length(info->var.xres_virtual,
						info->var.bits_per_pixel);
	//printk("line_length = %d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", info->fix.line_length);
	/* info->var.sync can be configure here. timing is not set here, strange */

	
	return 0;
}

    /*
     *  Set a single color register. The values supplied are already
     *  rounded down to the hardware's capabilities (according to the
     *  entries in the var structure). Return != 0 for invalid regno.
     */

static int vfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			 u_int transp, struct fb_info *info)
{
	if (regno >= 256)	/* no. of hw registers */
		return 1;
	/*
	 * Program hardware... do anything you want with transp
	 */

	/* grayscale works only partially under directcolor */
	if (info->var.grayscale) {
		/* grayscale = 0.30*R + 0.59*G + 0.11*B */
		red = green = blue =
		    (red * 77 + green * 151 + blue * 28) >> 8;
	}

	/* Directcolor:
	 *   var->{color}.offset contains start of bitfield
	 *   var->{color}.length contains length of bitfield
	 *   {hardwarespecific} contains width of RAMDAC
	 *   cmap[X] is programmed to (X << red.offset) | (X << green.offset) | (X << blue.offset)
	 *   RAMDAC[X] is programmed to (red, green, blue)
	 *
	 * Pseudocolor:
	 *    var->{color}.offset is 0 unless the palette index takes less than
	 *                        bits_per_pixel bits and is stored in the upper
	 *                        bits of the pixel value
	 *    var->{color}.length is set so that 1 << length is the number of available
	 *                        palette entries
	 *    cmap is not used
	 *    RAMDAC[X] is programmed to (red, green, blue)
	 *
	 * Truecolor:
	 *    does not use DAC. Usually 3 are present.
	 *    var->{color}.offset contains start of bitfield
	 *    var->{color}.length contains length of bitfield
	 *    cmap is programmed to (red << red.offset) | (green << green.offset) |
	 *                      (blue << blue.offset) | (transp << transp.offset)
	 *    RAMDAC does not exist
	 */
#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
	switch (info->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
	case FB_VISUAL_PSEUDOCOLOR:
		red = CNVT_TOHW(red, info->var.red.length);
		green = CNVT_TOHW(green, info->var.green.length);
		blue = CNVT_TOHW(blue, info->var.blue.length);
		transp = CNVT_TOHW(transp, info->var.transp.length);
		break;
	case FB_VISUAL_DIRECTCOLOR:
		red = CNVT_TOHW(red, 8);	/* expect 8 bit DAC */
		green = CNVT_TOHW(green, 8);
		blue = CNVT_TOHW(blue, 8);
		/* hey, there is bug in transp handling... */
		transp = CNVT_TOHW(transp, 8);
		break;
	}
#undef CNVT_TOHW
	/* Truecolor has hardware independent palette */
	if (info->fix.visual == FB_VISUAL_TRUECOLOR) {
		u32 v;

		if (regno >= 16)
			return 1;

		v = (red << info->var.red.offset) |
		    (green << info->var.green.offset) |
		    (blue << info->var.blue.offset) |
		    (transp << info->var.transp.offset);
		switch (info->var.bits_per_pixel) {
		case 8:
			break;
		case 16:
			((u32 *) (info->pseudo_palette))[regno] = v;
			break;
		case 24:
		case 32:
			((u32 *) (info->pseudo_palette))[regno] = v;
			break;
		}
		return 0;
	}
	return 0;
}

    /*
     *  Pan or Wrap the Display
     *
     *  This call looks only at xoffset, yoffset and the FB_VMODE_YWRAP flag
     */

static int vfb_pan_display(struct fb_var_screeninfo *var,
			   struct fb_info *info)
{
	fpga_lcd_info *lcd_info = (fpga_lcd_info *)info->par;
	
	if (var->vmode & FB_VMODE_YWRAP) {
		if (var->yoffset < 0
		    || var->yoffset >= info->var.yres_virtual
		    || var->xoffset)
			return -EINVAL;
	} else {
		if (var->xoffset + info->var.xres > info->var.xres_virtual ||
		    var->yoffset + info->var.yres > info->var.yres_virtual)
			return -EINVAL;
	}
	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;

	//printk("info->var.yoffset=%d,info->fix.line_length=%d,will set current address=0x%x\n",info->var.yoffset,info->fix.line_length,(VFR0_SCREEN_0_BASE_ADDRESS + info->var.yoffset*info->fix.line_length)&0x07ffffff);
	writel((lcd_info->fpga_frame_0_phy + info->var.yoffset*info->fix.line_length)&0x07ffffff, &lcd_info->fpga_ctlr_reg_virt_base->pb0_base);
	//printk("current addr=0x%x\n", readl(vfr0_remapped_base + pb0_base_addressoffset));
	
	if (var->vmode & FB_VMODE_YWRAP)
		info->var.vmode |= FB_VMODE_YWRAP;
	else
		info->var.vmode &= ~FB_VMODE_YWRAP;
	return 0;
}

    /*
     *  Most drivers don't need their own mmap function 
     */

static int vfb_mmap(struct fb_info *info,
		    struct vm_area_struct *vma)
{
#if defined(ALTERA_MMAP_MODE0)
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long page, pos;

	if (offset + size > info->fix.smem_len) {
		return -EINVAL;
	}

	pos = (unsigned long)info->fix.smem_start + offset;

	while (size > 0) {
		page = vmalloc_to_pfn((void *)pos);
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
		if (remap_pfn_range(vma, start, page, PAGE_SIZE, vma->vm_page_prot)) {
			return -EAGAIN;
		}
		start += PAGE_SIZE;
		pos += PAGE_SIZE;
		if (size > PAGE_SIZE)
			size -= PAGE_SIZE;
		else
			size = 0;
	}

#elif defined(ALTERA_MMAP_MODE1)
	u32 start = vma->vm_start;
	u32 size  = vma->vm_end - vma->vm_start;
	u32 offset = vma->vm_pgoff << PAGE_SHIFT;
	u32 phy = info->fix.smem_start + offset;
	u32  page,pos;
	int ret;

	if(phy + size > info->fix.smem_start + info->fix.smem_len)
	{
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if((ret = remap_pfn_range(vma, vma->vm_start, phy >> PAGE_SHIFT, size, vma->vm_page_prot))) 
	{
		printk(KERN_WARNING "%s: failed to remap_pfn_range\n", __func__);
		return -EAGAIN;     
	}
	
#endif
	return 0;

}

    /*
     *  Initialisation
     */

static int __devinit vfb_probe(struct platform_device *pdev)
{
	struct fb_info *info;
	int retval 				= -ENOMEM;
    	static char *mode_option 	= MODE_STRING;
	fpga_lcd_info * lcd_info;
	struct fb_var_screeninfo *var;
	
	info = framebuffer_alloc(sizeof(fpga_lcd_info), &pdev->dev);
	if (!info)
		goto err;

	info->fbops 	= &vfb_ops;
	info->flags 	= FBINFO_FLAG_DEFAULT;
	
	if(!(info->pseudo_palette = kzalloc(sizeof(u32) * 256, GFP_KERNEL)))
	{
		printk("alloc pseudo palette memory failed!\n");
		goto err1;
	}

	retval = fpga_controller_init(info);//xuewt new add
	if(retval <0)
	{
		printk("fpga controller init failed!\n");
		goto err1;
	}
	
	lcd_info = (fpga_lcd_info *)info->par;

#if defined(ALTERA_MMAP_MODE0)
	vfb_fix.smem_start = (unsigned long)lcd_info->mpu_frame_0_virt;
#elif defined(ALTERA_MMAP_MODE1)	
	vfb_fix.smem_start = FBSTARTADDR + VFR0_SCREEN_0_BASE_ADDRESS;
#endif
	info->screen_base = (char __iomem *)lcd_info->mpu_frame_0_virt;
	vfb_fix.smem_len 	= lcd_info->fpga_mpu_frame_len;
	info->fix 		= vfb_fix;//must before fb_find_mode

	if(lcd_info->lcd_mode_default)
	{
		printk("frame buffer using defaut 1080P mode\n");
		retval= fb_find_mode(&info->var, info, mode_option, NULL, 0, NULL, 8);/*it will call check var*/
		printk(KERN_INFO "Mode option %d\n", retval);
		if (!retval || (retval == 4))
			info->var = vfb_default;
	}
	else
	{
		printk("frame buffer using boot arg fbmode:xres=%d,yres=%d,bpp=%d,refresh=%d\n",lcd_info->xres, lcd_info->yres, lcd_info->bpp, lcd_info->refresh);
		var = &info->var;
		var->xres 			= lcd_info->xres;
		var->yres 			= lcd_info->yres;
		var->xres_virtual 		= lcd_info->xres;
		var->yres_virtual 		= lcd_info->yres;
		var->xoffset 			= 0;
		var->yoffset 			= 0;
		var->bits_per_pixel 	= lcd_info->bpp;
		var->pixclock 			= vfb_default.pixclock;
		var->left_margin 		= vfb_default.left_margin;
		var->right_margin 		= vfb_default.right_margin;
		var->upper_margin 	= vfb_default.upper_margin;
		var->lower_margin 	= vfb_default.lower_margin;
		var->hsync_len 		= vfb_default.hsync_len;
		var->vsync_len 		= vfb_default.vsync_len;
		var->vmode 			= FB_VMODE_NONINTERLACED;
	
		if (info->fbops->fb_check_var)
		{
			retval = info->fbops->fb_check_var(var, info);
			if(retval < 0)
			{
				goto err1;
			}
		}
	}
	
	retval= fb_alloc_cmap(&info->cmap, 256, 0);	
	if (retval < 0)
		
		goto err2;
	
	retval = register_framebuffer(info);	/* it will calculate the refresh rate according to timing parameters */

	if (retval < 0)
		goto err3;

	platform_set_drvdata(pdev, info);

	/* it will create file in the /sys/devices/platform/vfb.0/fbmode */
	retval = device_create_file(&pdev->dev, &altera_lcd_class_attrs[0]);//refer to w100fb.c
	if(retval)
	{
		printk(KERN_WARNING "fb%d: failed to register attributes (%d)\n", info->node, retval);
	}
		
	printk(KERN_INFO "fb%d: Altera frame buffer device, using 0x%x of video memory\n", info->node, (vfb_fix.smem_len) >> 10);

	return 0;

err3:
	fb_dealloc_cmap(&info->cmap);
err2:
	kfree(info->pseudo_palette);
err1:
	framebuffer_release(info);
err:

return retval;

}



static int vfb_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);
	fpga_lcd_info * lcd_info = (fpga_lcd_info *)info->par;

	if (info) {
		unregister_framebuffer(info);
		iounmap(lcd_info->fpga_ctlr_reg_virt_base);
		iounmap(lcd_info->mpu_frame_0_virt);
		fb_dealloc_cmap(&info->cmap);
		kfree(info->pseudo_palette);
		framebuffer_release(info);
	}
	return 0;
}

static struct platform_driver vfb_driver = {
	.probe	= vfb_probe,
	.remove = vfb_remove,
	.driver = {
		.name	= "vfb",
	},
};

static struct platform_device *vfb_device;

static int __init vfb_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&vfb_driver);

	if (!ret) {
		vfb_device = platform_device_alloc("vfb", 0);

		if (vfb_device)
			ret = platform_device_add(vfb_device);
		else
			ret = -ENOMEM;

		if (ret) {
			platform_device_put(vfb_device);
			platform_driver_unregister(&vfb_driver);
		}
	}

	return ret;
}

static void __exit vfb_exit(void)
{
	platform_device_unregister(vfb_device);
	platform_driver_unregister(&vfb_driver);
}
static int __init vfb_set_fb_mode(char *str)
{
	int ret;
	ret = altera_lcd_parse_mode(str, &xres, &yres, &bpp, &refresh);
	//printk("xres=%d,yres=%d,bpp=%d,refresh=%d\n", xres, yres, bpp, refresh);
	
	return ret;
}

__setup("fbmode=",vfb_set_fb_mode);

//module_init(custom_vip_driver_init);/* load this driver after uart driver */
late_initcall(custom_vip_driver_init);/* load this driver after uart driver */
module_exit(custom_vip_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver vip");
MODULE_AUTHOR("Nick Ni (nni@altera.com)");


