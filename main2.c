#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
	
#define  LIST_NODE_DATATYPE char *

#include "list.h"
#include "jpeglib.h"

#define PREV 0
#define NEXT 1
#define CURRENT 2 

#define RED   0
#define GREEN 1
#define BLUE  2

struct filenode
{
	char *name;
	int width;
	int height;
	int bpp;
};

struct imginfo
{
	int width;
	int height;
	int bpp;
};



int wait4touch(int ts)
{
	struct input_event buf;

	int action;
	while(1)
	{
		read(ts,&buf,sizeof(buf));

		if(buf.type == EV_ABS)
		{
			if(buf.code == ABS_X)	
			{
				action = buf.value >= 400 ? NEXT : PREV;
			}
		}
		if(buf.type == EV_KEY)
		{
			if(buf.code == BTN_TOUCH && buf.value == 0)	
				break;
		}
	}
	return action;
}

char *init_lcd(struct fb_var_screeninfo *pvinfo)
{
	int lcd = open("/dev/fb0",O_RDWR);

	if(lcd == -1)
	{
		perror("open /dev/fb0 falied ");
		exit(0);
	}

	bzero(pvinfo,sizeof(*pvinfo));
	ioctl(lcd,FBIOGET_VSCREENINFO,pvinfo);

	unsigned long bpp = pvinfo->bits_per_pixel;
	unsigned char *fbmem = mmap(NULL,pvinfo->xres *pvinfo->yres * bpp/8,
								PROT_READ|PROT_WRITE,MAP_SHARED,lcd,0);

	if(fbmem == MAP_FAILED)
	{
		perror("mmap() failed");
		exit(0);
	}
	return fbmem;
}

void write_lcd(unsigned char *fbmem,struct fb_var_screeninfo *pvinfo,
		unsigned char *rgb_buffer,struct imginfo *imageinfo)
{
	int x,y;
	for(y=0;y<imageinfo->height && y<pvinfo->yres;y++)
	{
		for(x=0;x<imageinfo->width && x<pvinfo->xres;x++)	
		{
			int image_offset = x*3+y*imageinfo->width*3;
			int lcd_offset = x*4+y*pvinfo->xres*4;

			memcpy(fbmem+lcd_offset+0,rgb_buffer+image_offset+BLUE,1);
			memcpy(fbmem+lcd_offset+1,rgb_buffer+image_offset+GREEN,1);
			memcpy(fbmem+lcd_offset+2,rgb_buffer+image_offset+RED,1);
		}
	}
}

static linklist head;

linklist creat_list(void)
{
	linklist head = init_list();
	return head;
}

int read_image_from_file(int fd,unsigned char *jpg_buf,unsigned long image_size)
{
	int nread = 0;
	int size = image_size,total_bytes = 0;
	unsigned char *buf = jpg_buf;

	while(size > 0)
	{
		nread = read(fd,buf,size);
		if(nread == -1)	
		{
			printf("read jpg failed \n");
			exit(0);	
		}
		size -= nread;
		buf += nread;
		total_bytes += nread;
	}
	return total_bytes;
}

linklist newnode(char *jpg_file_name)
{
	linklist new_list_node = calloc(1,sizeof(listnode));
	if(new_list_node == NULL)
	{
		perror("calloc () failed ");
		exit(0);	
	}
	new_list_node->data = jpg_file_name;
	new_list_node->next = new_list_node;
	new_list_node->prev = new_list_node;
	return new_list_node;
}

bool is_jpg(char *filename)
{
	if(strstr(filename,".jpg")==NULL && strstr(filename,".jpeg") == NULL)
		return false;

	return true;
}

linklist show_jpg(linklist last_jpg,int action,char *fbmem,
				  struct fb_var_screeninfo *pvinfo)
{
	if(last_jpg == NULL)
	{
		return NULL;	
	}

	if(fbmem == NULL)
	{
		return NULL;	
	}

	linklist jpg = last_jpg;

	switch(action)
	{
		case CURRENT : jpg = jpg;break;
		case PREV : 
				if(last_jpg->prev != head)			
					jpg = last_jpg->prev;
				else
					jpg = head->prev;
				break;
		case NEXT :
				if(last_jpg->next != head)
					jpg = last_jpg->next;
				else
					jpg = head->next;
				break;
	}

	//获取文件的属性
	struct stat file_info;
	stat(jpg->data,&file_info);

	//打开文件
	int fd = open(jpg->data,O_RDONLY);
	if(fd == -1 )
	{
		printf("open the %s failed\n",jpg->data);	
		return NULL;
	}

	char *jpgdata = calloc(1,file_info.st_size);
	int n = read_image_from_file(fd,jpgdata,file_info.st_size);
	
	//利用jpeglib的函数接口获取图片信息

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo,jpgdata,file_info.st_size);

	int ret = jpeg_read_header(&cinfo,true);
	if(ret != 1)
	{
		fprintf(stderr,"jpeg_read_header failed:%s\n",strerror(errno));
		return NULL;	
	}

	jpeg_start_decompress(&cinfo);
	struct imginfo jpgfile;
	jpgfile.width = cinfo.output_width;
	jpgfile.height = cinfo.output_height;
	jpgfile.bpp = cinfo.output_components *8;

	unsigned long rgb_size = jpgfile.width * jpgfile.height * jpgfile.bpp/8;
	unsigned char *rgb_buffer = calloc(1,rgb_size);

	int row_stride = jpgfile.width * jpgfile.bpp/8;
	while(cinfo.output_scanline < cinfo.output_height)
	{
			unsigned char *buff_array[1];
			buff_array[0] = rgb_buffer + (cinfo.output_scanline) * row_stride;
			jpeg_read_scanlines(&cinfo,buff_array,1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(jpgdata);

	write_lcd(fbmem,pvinfo,rgb_buffer,&jpgfile);
	return jpg;
}


int main(int argc,char **argv)
{
	if(argc > 2)
	{
		printf("invalid argument \n");
		exit(0);
	}

	char *target = argc==2 ? argv[1] : ".";

	struct stat fileinfo;
	bzero(&fileinfo,sizeof(fileinfo));
	stat(target,&fileinfo);

	struct fb_var_screeninfo vinfo;
	char *fbmem = init_lcd(&vinfo);

	head = creat_list();
	if(head == NULL)
		printf("create list failed");

	if(S_ISDIR(fileinfo.st_mode))
	{
		DIR *dp = opendir(target);
		chdir(target);	

		struct dirent *ep;
		while(1)
		{
			ep = readdir(dp);

			if(ep == NULL)
				break;
			bzero(&fileinfo,sizeof(fileinfo));
			stat(ep->d_name,&fileinfo);
			
			if(!S_ISREG(fileinfo.st_mode))
			{
				continue;
			}
			if(!is_jpg(ep->d_name))
			{
				continue;
			}
			linklist new = newnode(ep->d_name);
			if(new != NULL)
			{
				list_add(new,head);	
			}
		}
		if(head->next == head)
		{
			printf("there is no jpg in this dirent\n");
			exit(0);	
		}
		
		linklist jpg = head->next;
		int action = CURRENT; 
		
		int ts = open("/dev/input/event0",O_RDONLY);
		if(ts == -1)
		{
			perror("open /dev/input/event0");
			exit(0);		
		}

		while(1)
		{
			jpg = show_jpg(jpg,action,fbmem,&vinfo);
			action = wait4touch(ts);
		}
		
	}

	else if(S_ISREG(fileinfo.st_mode))
	{
		if(is_jpg(target))
		{
			linklist jpg_file = newnode(target);
			show_jpg(jpg_file,CURRENT,fbmem,&vinfo);
		}
		
		else
			printf("not a jpg file\n");
	}

	return 0;
}
