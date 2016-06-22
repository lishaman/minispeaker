#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#include "lcd_test.h"
#include "fonts.h"
#include "utils_interface.h"

int main(int argc, char *argv [])
{
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;
    int x = 0, y = 0;
    long int location = 0;
    unsigned char i, j;
    unsigned char str;
    unsigned int OffSet;
   
    char deviceName[64] = {}; 
    char macaddr[] = "255.255.255.255";
    char *ptr = deviceName;
    memset(macaddr, 0, sizeof (macaddr));

    sleep(5);
    get_mac_addr("wlan0", macaddr, "");
    sprintf(deviceName, "SmartAudio-%s", macaddr + 4); 


    /* 以读写方式打开显示设备节点文件 /dev/fb0 */
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd <= 0)
    {
        printf("Error: cannot open framebuffer device.\n");
        return -1;
    }
    //printf("The framebuffer device was opened successfully.\n");

    /* 得到framebuffer设备的固定屏幕信息 */
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo))
    {
        printf("Error reading fixed information.\n");
        return -2;
    }

    /* 得到可变屏幕信息 */
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
    {
        printf("Error reading variable information.\n");
        return -3;
    }

    /* 将映射的显存的大小 */
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    //printf("xres = %d, yres = %d, bpp = %d, screensize = %ld\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, screensize);

    /* 将显存映射到用户空间 */
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1)
    {
        printf("Error: failed to map framebuffer device to memory.\n");
        return -4;
    }
    //printf("The framebuffer device was mapped to memory successfully.\n");
#if 0
    /* 在显存上彩色画渐变矩形条*/
    for (y = 100; y < 200; y++)
    {
        for (x = 100; x < 200; x++)
        {
            location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;


            /* 简单处理,假定是32位或者是16位 565模式*/
            if (vinfo.bits_per_pixel == 32)
            {
#if 1
                *(fbp + location) = 100;    // Some blue
                *(fbp + location + 1) = 15+(x-100)/2;   // A little green
                *(fbp + location + 2) = 200-(y-100)/5; // A lot of red
                *(fbp + location + 3) = 0xff;  // No transparency
#endif
            }
            else
            { //assume 16bpp
                unsigned short b = 10;
                unsigned short g = (x-100)/6;
                unsigned short r = 31-(y-100)/16; // A lot of red
                unsigned short t = r<<11 | g << 5 | b;
                *((unsigned short *)(fbp + location)) = t;
            }
        }
    }
#endif
    x = 0;
    y = 128;

    while(1)
    {
        if(x == 400)
        {
            x = 0;
            y = y+16;
        }

        if (*ptr == 0)
        {
            break;
        }

        OffSet = (*ptr - 32)*16;
        for (i = 0; i < 16; i++)
        {
            str = *(ASCII_Table + OffSet + i);
            for (j = 0; j < 8; j++)
            {
                location = (x + j + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + i + vinfo.yoffset) * finfo.line_length;
                if ( str & (0x80>>j) )
                {
                    *(fbp + location) = 0x00;
                    *(fbp + location + 1) = 0x00;   // A little green
                    *(fbp + location + 2) = 0xff; // A lot of red
                    *(fbp + location + 3) = 0xff;  // No transparency
                }
            }
        }
        x += 8;
        ptr += 1;
    }

    munmap(fbp, screensize);
    close(fbfd);

    return 0;
}
