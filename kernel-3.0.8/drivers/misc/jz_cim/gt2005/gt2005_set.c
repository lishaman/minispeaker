#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>
#include "gt2005_camera.h"

#define dprintk(x...)   do{printk("gt2005-\t");printk(x);printk("\n");}while(0)

extern struct list_head sensor_list;

int gt2005_init(struct cim_sensor *sensor_info)
{
    //dprintk("gt2005--------------------gt2005_init_setting!");
    
    struct gt2005_sensor *s;
    struct i2c_client *client;
    struct cim_sensor *desc;
    int sensor_count = 0;
   
    s = container_of(sensor_info,struct gt2005_sensor,cs);
    list_for_each_entry(desc,&sensor_list,list){
	if(desc)
	   sensor_count++;	
    }
    client = s->client;

#ifdef CONFIG_GT2005_MIRROR    
    gt2005_write_reg16(client,0x0101 , 0x01);
#else
    gt2005_write_reg16(client,0x0101 , 0x00);
#endif
    gt2005_write_reg16(client,0x0103 , 0x00);


    gt2005_write_reg16(client,0x0105 , 0x00);
    gt2005_write_reg16(client,0x0106 , 0xF0);
    gt2005_write_reg16(client,0x0107 , 0x00);
    gt2005_write_reg16(client,0x0108 , 0x1C);

    gt2005_write_reg16(client,0x0109 , 0x01);
    gt2005_write_reg16(client,0x010A , 0x00);
    gt2005_write_reg16(client,0x010B , 0x00);
    gt2005_write_reg16(client,0x010C , 0x00);
    gt2005_write_reg16(client,0x010D , 0x08);
    gt2005_write_reg16(client,0x010E , 0x00);
    gt2005_write_reg16(client,0x010F , 0x08);
    gt2005_write_reg16(client,0x0110 , 0x06);
    gt2005_write_reg16(client,0x0111 , 0x40);
    gt2005_write_reg16(client,0x0112 , 0x04);
    gt2005_write_reg16(client,0x0113 , 0xB0);

    gt2005_write_reg16(client,0x0114 , 0x00);

    gt2005_write_reg16(client,0x0115 , 0x00);

    gt2005_write_reg16(client,0x0116 , 0x02);
    gt2005_write_reg16(client,0x0117 , 0x00);
    gt2005_write_reg16(client,0x0118 , 0x67);
    gt2005_write_reg16(client,0x0119 , 0x01);//02
    gt2005_write_reg16(client,0x011A , 0x04);
    gt2005_write_reg16(client,0x011B , 0x00);//01

    gt2005_write_reg16(client,0x011C , 0x00);

    gt2005_write_reg16(client,0x011D , 0x02);
    gt2005_write_reg16(client,0x011E , 0x00);

    gt2005_write_reg16(client,0x011F , 0x00);
    gt2005_write_reg16(client,0x0120 , 0x1C);
    gt2005_write_reg16(client,0x0121 , 0x00);
    gt2005_write_reg16(client,0x0122 , 0x04);
    gt2005_write_reg16(client,0x0123 , 0x00);
    gt2005_write_reg16(client,0x0124 , 0x00);
    gt2005_write_reg16(client,0x0125 , 0x00);
    gt2005_write_reg16(client,0x0126 , 0x00);
    gt2005_write_reg16(client,0x0127 , 0x00);
    gt2005_write_reg16(client,0x0128 , 0x00);

    //////gt2005_write_reg16(client,0x0200 , 0x00);
    gt2005_write_reg16(client,0x0200 , 0x10);
    //////gt2005_write_reg16(client,0x0201 , 0x00);	
    gt2005_write_reg16(client,0x0201 , 0x10);			  
    gt2005_write_reg16(client,0x0202 , 0x40);

    gt2005_write_reg16(client,0x0203 , 0x00);
    gt2005_write_reg16(client,0x0204 , 0x03);
    gt2005_write_reg16(client,0x0205 , 0x1F);
    gt2005_write_reg16(client,0x0206 , 0x0B);
    gt2005_write_reg16(client,0x0207 , 0x20);
    gt2005_write_reg16(client,0x0208 , 0x00);
    gt2005_write_reg16(client,0x0209 , 0x2A);
    gt2005_write_reg16(client,0x020A , 0x01);

    //////gt2005_write_reg16(client,0x020B , 0x48);
    //////gt2005_write_reg16(client,0x020C , 0x64);
    gt2005_write_reg16(client,0x020B , 0x30);
    gt2005_write_reg16(client,0x020C , 0x40);

    gt2005_write_reg16(client,0x020D , 0xC8);
    gt2005_write_reg16(client,0x020E , 0xBC);
    gt2005_write_reg16(client,0x020F , 0x08);
    //////gt2005_write_reg16(client,0x0210 , 0xd6);	
    gt2005_write_reg16(client,0x0210 , 0xF6);
    gt2005_write_reg16(client,0x0211 , 0x00);
    //////gt2005_write_reg16(client,0x0212 , 0x20);
    gt2005_write_reg16(client,0x0212 , 0x10);
    gt2005_write_reg16(client,0x0213 , 0x81);
    gt2005_write_reg16(client,0x0214 , 0x15);
    gt2005_write_reg16(client,0x0215 , 0x00);
    gt2005_write_reg16(client,0x0216 , 0x00);
    gt2005_write_reg16(client,0x0217 , 0x00);
    gt2005_write_reg16(client,0x0218 , 0x46);
    gt2005_write_reg16(client,0x0219 , 0x30);
    gt2005_write_reg16(client,0x021A , 0x03);
    gt2005_write_reg16(client,0x021B , 0x28);
    gt2005_write_reg16(client,0x021C , 0x02);
    gt2005_write_reg16(client,0x021D , 0x60);
    gt2005_write_reg16(client,0x021E , 0x00);
    gt2005_write_reg16(client,0x021F , 0x00);
    gt2005_write_reg16(client,0x0220 , 0x08);
    gt2005_write_reg16(client,0x0221 , 0x08);
    gt2005_write_reg16(client,0x0222 , 0x04);
    gt2005_write_reg16(client,0x0223 , 0x00);
    gt2005_write_reg16(client,0x0224 , 0x1F);
    gt2005_write_reg16(client,0x0225 , 0x1E);
    gt2005_write_reg16(client,0x0226 , 0x18);
    gt2005_write_reg16(client,0x0227 , 0x1D);
    gt2005_write_reg16(client,0x0228 , 0x1F);
    gt2005_write_reg16(client,0x0229 , 0x1F);
    gt2005_write_reg16(client,0x022A , 0x01);
    gt2005_write_reg16(client,0x022B , 0x04);
    gt2005_write_reg16(client,0x022C , 0x05);
    gt2005_write_reg16(client,0x022D , 0x05);
    gt2005_write_reg16(client,0x022E , 0x04);
    gt2005_write_reg16(client,0x022F , 0x03);
    gt2005_write_reg16(client,0x0230 , 0x02);
    gt2005_write_reg16(client,0x0231 , 0x1F);
    gt2005_write_reg16(client,0x0232 , 0x1A);
    gt2005_write_reg16(client,0x0233 , 0x19);
    gt2005_write_reg16(client,0x0234 , 0x19);
    gt2005_write_reg16(client,0x0235 , 0x1B);
    gt2005_write_reg16(client,0x0236 , 0x1F);
    gt2005_write_reg16(client,0x0237 , 0x04);
    gt2005_write_reg16(client,0x0238 , 0xEE);
    gt2005_write_reg16(client,0x0239 , 0xFF);
    gt2005_write_reg16(client,0x023A , 0x00);
    gt2005_write_reg16(client,0x023B , 0x00);
    gt2005_write_reg16(client,0x023C , 0x00);
    gt2005_write_reg16(client,0x023D , 0x00);
    gt2005_write_reg16(client,0x023E , 0x00);
    gt2005_write_reg16(client,0x023F , 0x00);
    gt2005_write_reg16(client,0x0240 , 0x00);
    gt2005_write_reg16(client,0x0241 , 0x00);
    gt2005_write_reg16(client,0x0242 , 0x00);
    gt2005_write_reg16(client,0x0243 , 0x21);
    gt2005_write_reg16(client,0x0244 , 0x42);
    gt2005_write_reg16(client,0x0245 , 0x53);
    gt2005_write_reg16(client,0x0246 , 0x54);
    gt2005_write_reg16(client,0x0247 , 0x54);
    gt2005_write_reg16(client,0x0248 , 0x54);
    gt2005_write_reg16(client,0x0249 , 0x33);
    gt2005_write_reg16(client,0x024A , 0x11);
    gt2005_write_reg16(client,0x024B , 0x00);
    gt2005_write_reg16(client,0x024C , 0x00);
    gt2005_write_reg16(client,0x024D , 0xFF);
    gt2005_write_reg16(client,0x024E , 0xEE);
    gt2005_write_reg16(client,0x024F , 0xDD);
    gt2005_write_reg16(client,0x0250 , 0x00);
    gt2005_write_reg16(client,0x0251 , 0x00);
    gt2005_write_reg16(client,0x0252 , 0x00);
    gt2005_write_reg16(client,0x0253 , 0x00);
    gt2005_write_reg16(client,0x0254 , 0x00);
    gt2005_write_reg16(client,0x0255 , 0x00);
    gt2005_write_reg16(client,0x0256 , 0x00);
    gt2005_write_reg16(client,0x0257 , 0x00);
    gt2005_write_reg16(client,0x0258 , 0x00);
    gt2005_write_reg16(client,0x0259 , 0x00);
    gt2005_write_reg16(client,0x025A , 0x00);
    gt2005_write_reg16(client,0x025B , 0x00);
    gt2005_write_reg16(client,0x025C , 0x00);
    gt2005_write_reg16(client,0x025D , 0x00);
    gt2005_write_reg16(client,0x025E , 0x00);
    gt2005_write_reg16(client,0x025F , 0x00);
    gt2005_write_reg16(client,0x0260 , 0x00);
    gt2005_write_reg16(client,0x0261 , 0x00);
    gt2005_write_reg16(client,0x0262 , 0x00);
    gt2005_write_reg16(client,0x0263 , 0x00);
    gt2005_write_reg16(client,0x0264 , 0x00);
    gt2005_write_reg16(client,0x0265 , 0x00);
    gt2005_write_reg16(client,0x0266 , 0x00);
    gt2005_write_reg16(client,0x0267 , 0x00);
    gt2005_write_reg16(client,0x0268 , 0x8F);
    gt2005_write_reg16(client,0x0269 , 0xA3);
    gt2005_write_reg16(client,0x026A , 0xB4);
    gt2005_write_reg16(client,0x026B , 0x90);
    gt2005_write_reg16(client,0x026C , 0x00);
    gt2005_write_reg16(client,0x026D , 0xD0);
    gt2005_write_reg16(client,0x026E , 0x60);
    gt2005_write_reg16(client,0x026F , 0xA0);
    gt2005_write_reg16(client,0x0270 , 0x40);
    gt2005_write_reg16(client,0x0300 , 0x81);
    gt2005_write_reg16(client,0x0301 , 0x80);
    gt2005_write_reg16(client,0x0302 , 0x22);
    gt2005_write_reg16(client,0x0303 , 0x06);
    gt2005_write_reg16(client,0x0304 , 0x03);
    gt2005_write_reg16(client,0x0305 , 0x83);
    gt2005_write_reg16(client,0x0306 , 0x00);
    gt2005_write_reg16(client,0x0307 , 0x22);
    gt2005_write_reg16(client,0x0308 , 0x00);
    gt2005_write_reg16(client,0x0309 , 0x55);
    gt2005_write_reg16(client,0x030A , 0x55);
    gt2005_write_reg16(client,0x030B , 0x55);
    gt2005_write_reg16(client,0x030C , 0x54);
    gt2005_write_reg16(client,0x030D , 0x1F);
    gt2005_write_reg16(client,0x030E , 0x0A);
    gt2005_write_reg16(client,0x030F , 0x10);
    gt2005_write_reg16(client,0x0310 , 0x04);
    gt2005_write_reg16(client,0x0311 , 0xFF);
    gt2005_write_reg16(client,0x0312 , 0x08);

    gt2005_write_reg16(client,0x0313 , 0x35);
    gt2005_write_reg16(client,0x0314 , 0x36);
    gt2005_write_reg16(client,0x0315 , 0x16);
    gt2005_write_reg16(client,0x0316 , 0x26);

    gt2005_write_reg16(client,0x0317 , 0x02);
    gt2005_write_reg16(client,0x0318 , 0x08);
    gt2005_write_reg16(client,0x0319 , 0x0C);

    gt2005_write_reg16(client,0x031A , 0x81);
    gt2005_write_reg16(client,0x031B , 0x00);
    gt2005_write_reg16(client,0x031C , 0x3D);
    gt2005_write_reg16(client,0x031D , 0x00);
    gt2005_write_reg16(client,0x031E , 0xF9);
    gt2005_write_reg16(client,0x031F , 0x00);
    gt2005_write_reg16(client,0x0320 , 0x24);
    gt2005_write_reg16(client,0x0321 , 0x14);
    gt2005_write_reg16(client,0x0322 , 0x1A);
    gt2005_write_reg16(client,0x0323 , 0x24);
    gt2005_write_reg16(client,0x0324 , 0x08);
    gt2005_write_reg16(client,0x0325 , 0xF0);
    gt2005_write_reg16(client,0x0326 , 0x30);
    gt2005_write_reg16(client,0x0327 , 0x17);
    gt2005_write_reg16(client,0x0328 , 0x11);
    gt2005_write_reg16(client,0x0329 , 0x22);
    gt2005_write_reg16(client,0x032A , 0x2F);
    gt2005_write_reg16(client,0x032B , 0x21);
    gt2005_write_reg16(client,0x032C , 0xDA);
    gt2005_write_reg16(client,0x032D , 0x10);
    gt2005_write_reg16(client,0x032E , 0xEA);
    gt2005_write_reg16(client,0x032F , 0x18);
    gt2005_write_reg16(client,0x0330 , 0x29);
    gt2005_write_reg16(client,0x0331 , 0x25);
    gt2005_write_reg16(client,0x0332 , 0x12);
    gt2005_write_reg16(client,0x0333 , 0x0F);
    gt2005_write_reg16(client,0x0334 , 0xE0);
    gt2005_write_reg16(client,0x0335 , 0x13);
    gt2005_write_reg16(client,0x0336 , 0xFF);
    gt2005_write_reg16(client,0x0337 , 0x20);
    gt2005_write_reg16(client,0x0338 , 0x46);
    gt2005_write_reg16(client,0x0339 , 0x04);
    gt2005_write_reg16(client,0x033A , 0x04);
    gt2005_write_reg16(client,0x033B , 0xFF);
    gt2005_write_reg16(client,0x033C , 0x01);
    gt2005_write_reg16(client,0x033D , 0x00);

    gt2005_write_reg16(client,0x033E , 0x03);
    gt2005_write_reg16(client,0x033F , 0x28);
    gt2005_write_reg16(client,0x0340 , 0x02);
    gt2005_write_reg16(client,0x0341 , 0x60);
    gt2005_write_reg16(client,0x0342 , 0xAC);
    gt2005_write_reg16(client,0x0343 , 0x97);
    gt2005_write_reg16(client,0x0344 , 0x7F);
    gt2005_write_reg16(client,0x0400 , 0xE8);
    gt2005_write_reg16(client,0x0401 , 0x40);
    gt2005_write_reg16(client,0x0402 , 0x00);
    gt2005_write_reg16(client,0x0403 , 0x00);
    gt2005_write_reg16(client,0x0404 , 0xF8);
    gt2005_write_reg16(client,0x0405 , 0x03);
    gt2005_write_reg16(client,0x0406 , 0x03);
    gt2005_write_reg16(client,0x0407 , 0x85);
    gt2005_write_reg16(client,0x0408 , 0x44);
    gt2005_write_reg16(client,0x0409 , 0x1F);
    gt2005_write_reg16(client,0x040A , 0x40);
    //////gt2005_write_reg16(client,0x040B , 0x33);  
    gt2005_write_reg16(client,0x040B , 0x33);  

    gt2005_write_reg16(client,0x040C , 0xA0);
    gt2005_write_reg16(client,0x040D , 0x00);
    gt2005_write_reg16(client,0x040E , 0x00);
    gt2005_write_reg16(client,0x040F , 0x00);
    gt2005_write_reg16(client,0x0410 , 0x0D);
    gt2005_write_reg16(client,0x0411 , 0x0D);
    gt2005_write_reg16(client,0x0412 , 0x0C);
    gt2005_write_reg16(client,0x0413 , 0x04);
    gt2005_write_reg16(client,0x0414 , 0x00);
    gt2005_write_reg16(client,0x0415 , 0x00);
    gt2005_write_reg16(client,0x0416 , 0x07);
    gt2005_write_reg16(client,0x0417 , 0x09);
    gt2005_write_reg16(client,0x0418 , 0x16);
    gt2005_write_reg16(client,0x0419 , 0x14);
    gt2005_write_reg16(client,0x041A , 0x11);
    gt2005_write_reg16(client,0x041B , 0x14);
    gt2005_write_reg16(client,0x041C , 0x07);
    gt2005_write_reg16(client,0x041D , 0x07);
    gt2005_write_reg16(client,0x041E , 0x06);
    gt2005_write_reg16(client,0x041F , 0x02);
    gt2005_write_reg16(client,0x0420 , 0x42);
    gt2005_write_reg16(client,0x0421 , 0x42);
    gt2005_write_reg16(client,0x0422 , 0x47);
    gt2005_write_reg16(client,0x0423 , 0x39);
    gt2005_write_reg16(client,0x0424 , 0x3E);
    gt2005_write_reg16(client,0x0425 , 0x4D);
    gt2005_write_reg16(client,0x0426 , 0x46);
    gt2005_write_reg16(client,0x0427 , 0x3A);
    gt2005_write_reg16(client,0x0428 , 0x21);
    gt2005_write_reg16(client,0x0429 , 0x21);
    gt2005_write_reg16(client,0x042A , 0x26);
    gt2005_write_reg16(client,0x042B , 0x1C);
    gt2005_write_reg16(client,0x042C , 0x25);
    gt2005_write_reg16(client,0x042D , 0x25);
    gt2005_write_reg16(client,0x042E , 0x28);
    gt2005_write_reg16(client,0x042F , 0x20);
    gt2005_write_reg16(client,0x0430 , 0x3E);
    gt2005_write_reg16(client,0x0431 , 0x3E);
    gt2005_write_reg16(client,0x0432 , 0x33);
    gt2005_write_reg16(client,0x0433 , 0x2E);
    gt2005_write_reg16(client,0x0434 , 0x54);
    gt2005_write_reg16(client,0x0435 , 0x53);
    gt2005_write_reg16(client,0x0436 , 0x3C);
    gt2005_write_reg16(client,0x0437 , 0x51);
    gt2005_write_reg16(client,0x0438 , 0x2B);
    gt2005_write_reg16(client,0x0439 , 0x2B);
    gt2005_write_reg16(client,0x043A , 0x38);
    gt2005_write_reg16(client,0x043B , 0x22);
    gt2005_write_reg16(client,0x043C , 0x3B);
    gt2005_write_reg16(client,0x043D , 0x3B);
    gt2005_write_reg16(client,0x043E , 0x31);
    gt2005_write_reg16(client,0x043F , 0x37);

    gt2005_write_reg16(client,0x0440 , 0x00);
    gt2005_write_reg16(client,0x0441 , 0x4B);
    gt2005_write_reg16(client,0x0442 , 0x00);
    gt2005_write_reg16(client,0x0443 , 0x00);
    gt2005_write_reg16(client,0x0444 , 0x31);

    gt2005_write_reg16(client,0x0445 , 0x00);
    gt2005_write_reg16(client,0x0446 , 0x00);
    gt2005_write_reg16(client,0x0447 , 0x00);
    gt2005_write_reg16(client,0x0448 , 0x00);
    gt2005_write_reg16(client,0x0449 , 0x00);
    gt2005_write_reg16(client,0x044A , 0x00);
    gt2005_write_reg16(client,0x044D , 0xE0);
    gt2005_write_reg16(client,0x044E , 0x05);
    gt2005_write_reg16(client,0x044F , 0x07);
    gt2005_write_reg16(client,0x0450 , 0x00);
    gt2005_write_reg16(client,0x0451 , 0x00);
    gt2005_write_reg16(client,0x0452 , 0x00);
    gt2005_write_reg16(client,0x0453 , 0x00);
    gt2005_write_reg16(client,0x0454 , 0x00);
    gt2005_write_reg16(client,0x0455 , 0x00);
    gt2005_write_reg16(client,0x0456 , 0x00);
    gt2005_write_reg16(client,0x0457 , 0x00);
    gt2005_write_reg16(client,0x0458 , 0x00);
    gt2005_write_reg16(client,0x0459 , 0x00);
    gt2005_write_reg16(client,0x045A , 0x00);
    gt2005_write_reg16(client,0x045B , 0x00);
    gt2005_write_reg16(client,0x045C , 0x00);
    gt2005_write_reg16(client,0x045D , 0x00);
    gt2005_write_reg16(client,0x045E , 0x00);
    gt2005_write_reg16(client,0x045F , 0x00);

    gt2005_write_reg16(client,0x0460 , 0x80);
    gt2005_write_reg16(client,0x0461 , 0x10);
    gt2005_write_reg16(client,0x0462 , 0x10);
    gt2005_write_reg16(client,0x0463 , 0x10);
    gt2005_write_reg16(client,0x0464 , 0x08);
    gt2005_write_reg16(client,0x0465 , 0x08);
    gt2005_write_reg16(client,0x0466 , 0x11);
    gt2005_write_reg16(client,0x0467 , 0x09);
    gt2005_write_reg16(client,0x0468 , 0x23);
    gt2005_write_reg16(client,0x0469 , 0x2A);
    gt2005_write_reg16(client,0x046A , 0x2A);
    gt2005_write_reg16(client,0x046B , 0x47);
    gt2005_write_reg16(client,0x046C , 0x52);
    gt2005_write_reg16(client,0x046D , 0x42);
    gt2005_write_reg16(client,0x046E , 0x36);
    gt2005_write_reg16(client,0x046F , 0x46);
    gt2005_write_reg16(client,0x0470 , 0x3A);
    gt2005_write_reg16(client,0x0471 , 0x32);
    gt2005_write_reg16(client,0x0472 , 0x32);
    gt2005_write_reg16(client,0x0473 , 0x38);
    gt2005_write_reg16(client,0x0474 , 0x3D);
    gt2005_write_reg16(client,0x0475 , 0x2F);
    gt2005_write_reg16(client,0x0476 , 0x29);
    gt2005_write_reg16(client,0x0477 , 0x48);

    gt2005_write_reg16(client,0x0600 , 0x00);
    gt2005_write_reg16(client,0x0601 , 0x24);
    gt2005_write_reg16(client,0x0602 , 0x45);
    gt2005_write_reg16(client,0x0603 , 0x0E);
    gt2005_write_reg16(client,0x0604 , 0x14);
    gt2005_write_reg16(client,0x0605 , 0x2F);
    gt2005_write_reg16(client,0x0606 , 0x01);
    gt2005_write_reg16(client,0x0607 , 0x0E);
    gt2005_write_reg16(client,0x0608 , 0x0E);
    gt2005_write_reg16(client,0x0609 , 0x37);
    gt2005_write_reg16(client,0x060A , 0x18);
    gt2005_write_reg16(client,0x060B , 0xA0);
    gt2005_write_reg16(client,0x060C , 0x20);
    gt2005_write_reg16(client,0x060D , 0x07);
    gt2005_write_reg16(client,0x060E , 0x47);
    gt2005_write_reg16(client,0x060F , 0x90);
    gt2005_write_reg16(client,0x0610 , 0x06);
    gt2005_write_reg16(client,0x0611 , 0x0C);
    gt2005_write_reg16(client,0x0612 , 0x28);
    gt2005_write_reg16(client,0x0613 , 0x13);
    gt2005_write_reg16(client,0x0614 , 0x0B);
    gt2005_write_reg16(client,0x0615 , 0x10);
    gt2005_write_reg16(client,0x0616 , 0x14);
    gt2005_write_reg16(client,0x0617 , 0x19);
    gt2005_write_reg16(client,0x0618 , 0x52);
    gt2005_write_reg16(client,0x0619 , 0xA0);
    gt2005_write_reg16(client,0x061A , 0x11);
    gt2005_write_reg16(client,0x061B , 0x33);
    gt2005_write_reg16(client,0x061C , 0x56);
    gt2005_write_reg16(client,0x061D , 0x20);
    gt2005_write_reg16(client,0x061E , 0x28);
    gt2005_write_reg16(client,0x061F , 0x2B);
    gt2005_write_reg16(client,0x0620 , 0x22);
    gt2005_write_reg16(client,0x0621 , 0x11);
    gt2005_write_reg16(client,0x0622 , 0x75);
    gt2005_write_reg16(client,0x0623 , 0x49);
    gt2005_write_reg16(client,0x0624 , 0x6E);
    gt2005_write_reg16(client,0x0625 , 0x80);
    gt2005_write_reg16(client,0x0626 , 0x02);
    gt2005_write_reg16(client,0x0627 , 0x0C);
    gt2005_write_reg16(client,0x0628 , 0x51);
    gt2005_write_reg16(client,0x0629 , 0x25);
    gt2005_write_reg16(client,0x062A , 0x01);
    gt2005_write_reg16(client,0x062B , 0x3D);
    gt2005_write_reg16(client,0x062C , 0x04);
    gt2005_write_reg16(client,0x062D , 0x01);
    gt2005_write_reg16(client,0x062E , 0x0C);
    gt2005_write_reg16(client,0x062F , 0x2C);
    gt2005_write_reg16(client,0x0630 , 0x0D);
    gt2005_write_reg16(client,0x0631 , 0x14);
    gt2005_write_reg16(client,0x0632 , 0x12);
    gt2005_write_reg16(client,0x0633 , 0x34);
    gt2005_write_reg16(client,0x0634 , 0x00);
    gt2005_write_reg16(client,0x0635 , 0x00);
    gt2005_write_reg16(client,0x0636 , 0x00);
    gt2005_write_reg16(client,0x0637 , 0xB1);
    gt2005_write_reg16(client,0x0638 , 0x22);
    gt2005_write_reg16(client,0x0639 , 0x32);
    gt2005_write_reg16(client,0x063A , 0x0E);
    gt2005_write_reg16(client,0x063B , 0x18);
    gt2005_write_reg16(client,0x063C , 0x88);
    gt2005_write_reg16(client,0x0640 , 0xB2);
    gt2005_write_reg16(client,0x0641 , 0xC0);
    gt2005_write_reg16(client,0x0642 , 0x01);
    gt2005_write_reg16(client,0x0643 , 0x26);
    gt2005_write_reg16(client,0x0644 , 0x13);
    gt2005_write_reg16(client,0x0645 , 0x88);
    gt2005_write_reg16(client,0x0646 , 0x64);
    gt2005_write_reg16(client,0x0647 , 0x00);
    gt2005_write_reg16(client,0x0681 , 0x1B);
    gt2005_write_reg16(client,0x0682 , 0xA0);
    gt2005_write_reg16(client,0x0683 , 0x28);
    gt2005_write_reg16(client,0x0684 , 0x00);
    gt2005_write_reg16(client,0x0685 , 0xB0);
    gt2005_write_reg16(client,0x0686 , 0x6F);
    gt2005_write_reg16(client,0x0687 , 0x33);
    gt2005_write_reg16(client,0x0688 , 0x1F);
    gt2005_write_reg16(client,0x0689 , 0x44);
    gt2005_write_reg16(client,0x068A , 0xA8);
    gt2005_write_reg16(client,0x068B , 0x44);
    gt2005_write_reg16(client,0x068C , 0x08);
    gt2005_write_reg16(client,0x068D , 0x08);
    gt2005_write_reg16(client,0x068E , 0x00);
    gt2005_write_reg16(client,0x068F , 0x00);
    gt2005_write_reg16(client,0x0690 , 0x01);
    gt2005_write_reg16(client,0x0691 , 0x00);
    gt2005_write_reg16(client,0x0692 , 0x01);
    gt2005_write_reg16(client,0x0693 , 0x00);
    gt2005_write_reg16(client,0x0694 , 0x00);
    gt2005_write_reg16(client,0x0695 , 0x00);
    gt2005_write_reg16(client,0x0696 , 0x00);
    gt2005_write_reg16(client,0x0697 , 0x00);
    gt2005_write_reg16(client,0x0698 , 0x2A);
    gt2005_write_reg16(client,0x0699 , 0x80);
    gt2005_write_reg16(client,0x069A , 0x1F);
    gt2005_write_reg16(client,0x069B , 0x00);
    gt2005_write_reg16(client,0x069C , 0x02);
    gt2005_write_reg16(client,0x069D , 0xF5);
    gt2005_write_reg16(client,0x069E , 0x03);
    gt2005_write_reg16(client,0x069F , 0x6D);
    gt2005_write_reg16(client,0x06A0 , 0x0C);
    gt2005_write_reg16(client,0x06A1 , 0xB8);
    gt2005_write_reg16(client,0x06A2 , 0x0D);
    gt2005_write_reg16(client,0x06A3 , 0x74);
    gt2005_write_reg16(client,0x06A4 , 0x00);
    gt2005_write_reg16(client,0x06A5 , 0x2F);
    gt2005_write_reg16(client,0x06A6 , 0x00);
    gt2005_write_reg16(client,0x06A7 , 0x2F);
    gt2005_write_reg16(client,0x0F00 , 0x00);
    gt2005_write_reg16(client,0x0F01 , 0x00);

    gt2005_write_reg16(client,0x0100 , 0x01);
    gt2005_write_reg16(client,0x0102 , 0x02);
    gt2005_write_reg16(client,0x0104 , 0x03);

    gt2005_write_reg16(client,0x0116, 0x02 );
    gt2005_write_reg16(client,0x0118, 0x34 ); // PLL_MULTI
    gt2005_write_reg16(client,0x0119, 0x01 );//01
    gt2005_write_reg16(client,0x011A, 0x04 ); 
    gt2005_write_reg16(client,0x011B, 0x00 );//00

    gt2005_write_reg16(client,0x0109, 0x00 );
    gt2005_write_reg16(client,0x010a, 0x04 );// line binning 1/2
    gt2005_write_reg16(client,0x010B, 0x03 ); 
    gt2005_write_reg16(client,0x0110, 0x01); //Horizontal output size 352
    gt2005_write_reg16(client,0x0111, 0x60);
    gt2005_write_reg16(client,0x0112, 0x01);//Vertical output size 288
    gt2005_write_reg16(client,0x0113, 0x20);
}

int gt2005_preview_set(struct cim_sensor *sensor_info)
{
    struct gt2005_sensor *s;
    struct i2c_client *client;
    s = container_of(sensor_info,struct gt2005_sensor,cs);
    client = s->client;

    dprintk("-----gt2005_preview_set %s %d\n", __FUNCTION__, __LINE__);
    gt2005_write_reg16(client,0x040B , 0x22);  //denoise  lower
    gt2005_write_reg16(client,0x020B , 0x30);  // white side enhance
    gt2005_write_reg16(client,0x020C , 0x40);  // black side enhance
    //recover shutter registers
    gt2005_write_reg16(client,0x0300 , 0x81);
    gt2005_write_reg16(client,0x0304 , 0x03);
    gt2005_write_reg16(client,0x0305 , 0x83);
    gt2005_write_reg16(client,0x0306 , 0x00);
    gt2005_write_reg16(client,0x0307 , 0x22);
    gt2005_write_reg16(client,0x0308 , 0x00);
    return 0;
}

static void set_fps(struct i2c_client *client,int fps)
{
    dprintk("--------set fps : %d (only small resolutions)",fps);
    /*
    if((cur_width > 800)||(cur_height > 600))
    {
        dprintk("-----------------can't surpport the resolution");
        return;
    }*/
    switch(fps)
    {
        case 15:
            gt2005_write_reg16(client,0x0313 , 0x34); 
            gt2005_write_reg16(client,0x0314 , 0x3B);// step

            gt2005_write_reg16(client,0x0116, 0x02); 
            gt2005_write_reg16(client,0x0118, 0x34);// PLL_MULTI
            gt2005_write_reg16(client,0x0119, 0x01); 
            gt2005_write_reg16(client,0x011A, 0x04); 
            gt2005_write_reg16(client,0x011B, 0x00); 

            gt2005_write_reg16(client,0x0109, 0x00);

            gt2005_write_reg16(client,0x010a, 0x04);// line binning 1/2
            gt2005_write_reg16(client,0x010b, 0x03); 
            gt2005_write_reg16(client,0x0312 ,0x08); //disable auto fps
            break;
        case 20:
            gt2005_write_reg16(client,0x0313 , 0x35); 
            gt2005_write_reg16(client,0x0314 , 0x9e);// step/////////  9d
            gt2005_write_reg16(client,0x0116, 0x02); 
            gt2005_write_reg16(client,0x0118, 0x45);// PLL_MULTI
            gt2005_write_reg16(client,0x0119, 0x01); 
            gt2005_write_reg16(client,0x011A, 0x04); 
            gt2005_write_reg16(client,0x011B, 0x00); 
            gt2005_write_reg16(client,0x0109, 0x00);		
            gt2005_write_reg16(client,0x010a, 0x04);// line binning 1/2
            gt2005_write_reg16(client,0x010b, 0x03); 
            gt2005_write_reg16(client,0x0312 ,0x08); //disable auto fps
            break;
        case 25:
            gt2005_write_reg16(client,0x0313 , 0x36); 
            gt2005_write_reg16(client,0x0314 , 0xff);// step

            gt2005_write_reg16(client,0x0116, 0x02); 
            gt2005_write_reg16(client,0x0118, 0x56);// PLL_MULTI
            gt2005_write_reg16(client,0x0119, 0x01); 
            gt2005_write_reg16(client,0x011A, 0x04); 
            gt2005_write_reg16(client,0x011B, 0x00); 
            gt2005_write_reg16(client,0x0109, 0x00);			
            gt2005_write_reg16(client,0x010a, 0x04);// line binning 1/2
            gt2005_write_reg16(client,0x010b, 0x03); 
            gt2005_write_reg16(client,0x0312 ,0x08); //disable auto fps
            break;
        case 30:		 	
            /*gt2005_write_reg16(client,0x0313 , 0x38);
              gt2005_write_reg16(client,0x0314 , 0x76);// step

              gt2005_write_reg16(client,0x0116, 0x02 );
              gt2005_write_reg16(client,0x0118, 0x68 ); // PLL_MULTI
              gt2005_write_reg16(client,0x0119, 0x01 );
              gt2005_write_reg16(client,0x011A, 0x04 );
              gt2005_write_reg16(client,0x011B, 0x00 );
              gt2005_write_reg16(client,0x0109, 0x00 );
              gt2005_write_reg16(client,0x010a, 0x04 );// line binning 1/2
              gt2005_write_reg16(client,0x010b, 0x03 );
              gt2005_write_reg16(client,0x0312 ,0x08); *///disable auto fps
            break;
        default:
            dprintk("--------other fps : %d",fps);
            break;
    }
}

static void set_size_1600x1200(struct i2c_client *client,int mode)
{

    dprintk("---------------%d------------------------  1600x1200!",mode);

    gt2005_write_reg16(client,0x0109, 0x00);
    gt2005_write_reg16(client,0x010a, 0x00);// line binning 1/2
    gt2005_write_reg16(client,0x010b, 0x00); 

    gt2005_write_reg16(client,0x0110, 0x06 );//Horizontal output size 1600
    gt2005_write_reg16(client,0x0111, 0x40 );
    gt2005_write_reg16(client,0x0112, 0x04 );//Vertical output size 1200
    gt2005_write_reg16(client,0x0113, 0xb0 );


}

static void set_size_1280x720(struct i2c_client *client,int mode)
{
    dprintk("---------------------------------------  1280x720!");
    dprintk("1280x720  only 17fps,can't setup other value !");

    gt2005_write_reg16(client,0x0105 , 0x00);
    gt2005_write_reg16(client,0x0106 , 0xf0);
    gt2005_write_reg16(client,0x0107 , 0x00);
    gt2005_write_reg16(client,0x0108 , 0x1a);
    gt2005_write_reg16(client,0x0313 , 0x38);
    gt2005_write_reg16(client,0x0314 , 0xf3);// step

    gt2005_write_reg16(client,0x0116, 0x02);
    gt2005_write_reg16(client,0x0118, 0x6e);
    gt2005_write_reg16(client,0x0119, 0x01);
    gt2005_write_reg16(client,0x011A, 0x04);
    gt2005_write_reg16(client,0x011B, 0x00);

    gt2005_write_reg16(client,0x0109, 0x00);
    gt2005_write_reg16(client,0x010a, 0x00);
    gt2005_write_reg16(client,0x010b, 0x0f);
    gt2005_write_reg16(client,0x0312, 0x08); //disable auto fps		
    //17.15fps
    gt2005_write_reg16(client,0x0110, 0x05 );//Horizontal output size 1280
    gt2005_write_reg16(client,0x0111, 0x00 );
    gt2005_write_reg16(client,0x0112, 0x02 );//Vertical output size 720
    gt2005_write_reg16(client,0x0113, 0xd0 );
    msleep(50);

}


static void set_size_1024x768(struct i2c_client *client,int mode)
{

    dprintk("---------------------------------------  1024x768!");

    gt2005_write_reg16(client,0x0109, 0x00);
    gt2005_write_reg16(client,0x010a, 0x00);// line binning 1/2
    gt2005_write_reg16(client,0x010b, 0x0f);

    gt2005_write_reg16(client,0x0110, 0x04 ); //Horizontal output size 1024
    gt2005_write_reg16(client,0x0111, 0x00 );
    gt2005_write_reg16(client,0x0112, 0x03 ); //Vertical output size 768
    gt2005_write_reg16(client,0x0113, 0x00 );


}

static void set_size_800x600(struct i2c_client *client,int mode)
{
    dprintk("---------------------------------------  800x600!");

    gt2005_write_reg16(client,0x0110, 0x03 );//Horizontal output size 800
    gt2005_write_reg16(client,0x0111, 0x20 );
    gt2005_write_reg16(client,0x0112, 0x02 );//Vertical output size 600
    gt2005_write_reg16(client,0x0113, 0x58 );
    set_fps(client,20);

} 

static void set_size_720x576(struct i2c_client *client,int mode)
{
    dprintk("--------------------------------------- 720x576!");

    gt2005_write_reg16(client,0x0110, 0x02 ); //Horizontal output size 720
    gt2005_write_reg16(client,0x0111, 0xd0 );
    gt2005_write_reg16(client,0x0112, 0x02 ); //Vertical output size 576
    gt2005_write_reg16(client,0x0113, 0x40 );
    set_fps(client,20);

}  
static void set_size_720x480(struct i2c_client *client,int mode)
{
    dprintk("--------------------------------------- 720x480!");
    gt2005_write_reg16(client,0x0110, 0x02 ); //Horizontal output size 720
    gt2005_write_reg16(client,0x0111, 0xd0 );
    gt2005_write_reg16(client,0x0112, 0x01 ); //Vertical output size 480
    gt2005_write_reg16(client,0x0113, 0xE0 );
    set_fps(client,20);

}  

static void set_size_640x480(struct i2c_client *client,int mode)
{
    dprintk("--------------------------------------- 640x480!");

    gt2005_write_reg16(client,0x0110, 0x02 ); //Horizontal output size 640
    gt2005_write_reg16(client,0x0111, 0x80 );
    gt2005_write_reg16(client,0x0112, 0x01 ); //Vertical output size 480
    gt2005_write_reg16(client,0x0113, 0xe0 );
    set_fps(client,25);
}  

static void set_size_352x288(struct i2c_client * client, int mode)
{
    dprintk("--------------------------------------- 352x288!");

    gt2005_write_reg16(client,0x0110, 0x01); //Horizontal output size 352
    gt2005_write_reg16(client,0x0111, 0x60);
    gt2005_write_reg16(client,0x0112, 0x01);//Vertical output size 288
    gt2005_write_reg16(client,0x0113, 0x20);
    set_fps(client,25);
}

static void set_size_320x240(struct i2c_client *client,int mode)
{
    dprintk("--------------------------------------- 320x240!");
    /*gt2005_write_reg16(client,0x0313 , 0x32); 
      gt2005_write_reg16(client,0x0314 , 0x9a);// step

      gt2005_write_reg16(client,0x0116, 0x02); 
      gt2005_write_reg16(client,0x0118, 0x20);// PLL_MULTI
      gt2005_write_reg16(client,0x0119, 0x01); 
      gt2005_write_reg16(client,0x011A, 0x04); 
      gt2005_write_reg16(client,0x011B, 0x00); 

      gt2005_write_reg16(client,0x0109, 0x00); 
      gt2005_write_reg16(client,0x010a, 0x04);// line binning 1/4
      gt2005_write_reg16(client,0x010B, 0x03); */
    gt2005_write_reg16(client,0x0110, 0x01); //Horizontal output size 320
    gt2005_write_reg16(client,0x0111, 0x40);
    gt2005_write_reg16(client,0x0112, 0x00);//Vertical output size 240
    gt2005_write_reg16(client,0x0113, 0xf0);
}  

static void set_size_176x144(struct i2c_client * client, int mode) 
{
    dprintk("--------------------------------------- 176x144!");

    gt2005_write_reg16(client,0x0110, 0x00); //Horizontal output size 176
    gt2005_write_reg16(client,0x0111, 0xb0);
    gt2005_write_reg16(client,0x0112, 0x00);//Vertical output size 144
    gt2005_write_reg16(client,0x0113, 0x90);
    set_fps(client,25);
}


int gt2005_size_switch(struct cim_sensor *sensor_info,int width,int height)
{
    struct gt2005_sensor *s;
    struct i2c_client *client;
    s = container_of(sensor_info,struct gt2005_sensor,cs);
    client = s->client;	
    //    dprintk("%dx%d - mode(%d)",width,height,setmode);
    int setmode = 20;
    //cur_width = width;
    //cur_height =height;
    if(width == 1600 && height == 1200)
    {
        set_size_1600x1200(client,setmode);
    }
    else if(width == 1280 && height == 720)
    {
        set_size_1280x720(client,setmode);
    }

    else if(width == 1024 && height == 768)
    {
        set_size_1024x768(client,setmode);
    }
    else if(width == 800 && height == 600)
    {
        set_size_800x600(client,setmode);
    }
    else if(width == 720 && height == 576)
    {
        set_size_720x576(client,setmode);
    }
    else if(width == 720 && height == 480)
    {
        set_size_720x480(client,setmode);
    }
    else if(width == 640 && height == 480)
    {
        set_size_640x480(client,setmode);
    }
    else if(width == 352 && height == 288)
    {
        set_size_352x288(client,setmode);
    }
    else if(width == 320 && height == 240)
    {
        set_size_320x240(client,setmode);
    }
    else if(width == 176 && height == 144)
    {
        set_size_176x144(client,setmode);
    }
    else
        return 0;
}

static void gt2005_write_shutter(struct i2c_client *client,unsigned short shutter)
{
    unsigned short AGain_shutter,DGain_shutter;
    dprintk("----------------------------gt2005__write_shutter");
    AGain_shutter = (((unsigned short)gt2005_read_reg16(client,0x0014))<<8 )|( gt2005_read_reg16(client,0x0015));
    DGain_shutter = (((unsigned short)gt2005_read_reg16(client,0x0016))<<8 )|( gt2005_read_reg16(client,0x0017));

    gt2005_write_reg16(client,0x0300 , 0x41); //close ALC

    gt2005_write_reg16(client,0x0305 , shutter&0xff);           
    gt2005_write_reg16(client,0x0304 , (shutter>>8)&0xff); 

    gt2005_write_reg16(client,0x0307 , AGain_shutter&0xff);      
    gt2005_write_reg16(client,0x0306 , (AGain_shutter>>8)&0xff); //AG

    gt2005_write_reg16(client,0x0308,  (DGain_shutter>>2)&0xff);   //DG

}  

static unsigned short gt2005_read_shutter(struct i2c_client *client)
{
    unsigned char  temp_reg2 = 0;
    unsigned short temp_reg1 = 0,shutter = 0;
    dprintk("-------------------------gt2005__read_shutter");
    //Backup the preview mode last shutter & sensor gain. 
    temp_reg1 = gt2005_read_reg16(client,0x0012);
    temp_reg2 = gt2005_read_reg16(client,0x0013);
    shutter = (temp_reg1 << 8) | (temp_reg2 & 0xFF);	
    return shutter;
}  

int gt2005_capture_set(struct cim_sensor *sensor_info)
{
    struct gt2005_sensor *s;
    struct i2c_client *client;
    s = container_of(sensor_info,struct gt2005_sensor,cs);
    client = s->client;

    unsigned short shutter ;
    dprintk("------gt2005_capture_set %s %d\n", __FUNCTION__, __LINE__);
    gt2005_write_reg16(client,0x0312,  0xa8);  // auto fps
    gt2005_write_reg16(client,0x040B , 0x22);  //denoise
    gt2005_write_reg16(client,0x020B , 0x58);  // white side enhance
    gt2005_write_reg16(client,0x020C , 0x68);  // black side enhance
    shutter =gt2005_read_shutter(client);
    gt2005_write_shutter(client,shutter);
    return 0;
}

void gt2005_set_ab_50hz(struct i2c_client *client)
{
    dprintk("--------------------------ab_50hz ");
}

void gt2005_set_ab_60hz(struct i2c_client *client)
{
    dprintk("--------------------------ab_60hz ");
}

int gt2005_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
    dprintk("--------------------------gt2005_set_antibanding ");	
}

void gt2005_set_effect_normal(struct i2c_client *client)
{
    gt2005_write_reg16(client,0x0115 , 0x00);
    dprintk("--------------------------effect_normal ");
}

void gt2005_set_effect_grayscale(struct i2c_client *client)
{
    gt2005_write_reg16(client,0x0115, 0x06);
    dprintk("--------------------------effect_grayscale ");
}

void gt2005_set_effect_sepia(struct i2c_client *client)
{
    gt2005_write_reg16(client,0x0115, 0x0A);
    dprintk("--------------------------effect_sepia ");
}

void gt2005_set_effect_colorinv(struct i2c_client *client)
{
    gt2005_write_reg16(client,0x0115, 0x09);
    dprintk("--------------------------effect_colorinv ");
}

void gt2005_set_effect_sepiagreen(struct i2c_client *client)
{
    dprintk("--------------------------effect_sepiagreen ");
}

void gt2005_set_effect_sepiablue(struct i2c_client *client)
{
    dprintk("--------------------------effect_sepiablue ");
}

int gt2005_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct gt2005_sensor *s;
	struct i2c_client * client;
	unsigned char *str_effect;

	s = container_of(sensor_info, struct gt2005_sensor, cs);
	client = s->client;
	switch(arg)
	{
		case EFFECT_NONE:
			gt2005_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
		case EFFECT_MONO :
			gt2005_set_effect_grayscale(client);
			str_effect = "EFFECT_MONO";
			break;
		case EFFECT_NEGATIVE :
			gt2005_set_effect_colorinv(client);
			str_effect = "EFFECT_NEGATIVE";
			break;
		case EFFECT_SOLARIZE ://bao guang
			str_effect = "EFFECT_SOLARIZE";
			break;
		case EFFECT_SEPIA :
			gt2005_set_effect_sepia(client);
			str_effect = "EFFECT_SEPIA";
			break;
		case EFFECT_POSTERIZE ://se diao fen li
			str_effect = "EFFECT_POSTERIZE";
			break;
		case EFFECT_WHITEBOARD :
			str_effect = "EFFECT_WHITEBOARD";
			break;
		case EFFECT_BLACKBOARD :
			str_effect = "EFFECT_BLACKBOARD";
			break;
		case EFFECT_AQUA  ://qian lv se
			gt2005_set_effect_sepiagreen(client);
			str_effect = "EFFECT_AQUA";
			break;
		case EFFECT_PASTEL:
			str_effect = "EFFECT_PASTEL";
			break;
		case EFFECT_MOSAIC:
			str_effect = "EFFECT_MOSAIC";
			break;
		case EFFECT_RESIZE:
			str_effect = "EFFECT_RESIZE";
			break;
		default:
			gt2005_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
	}
	return 0;	
}

void gt2005_set_wb_auto(struct i2c_client *client)
{
    gt2005_write_reg16(client,0x031A , 0x81);
    gt2005_write_reg16(client,0x0320,  0x24);
    gt2005_write_reg16(client,0x0321,  0x14);
    gt2005_write_reg16(client,0x0322,  0x1a);
    gt2005_write_reg16(client,0x0323 , 0x24);
    gt2005_write_reg16(client,0x0441,  0x4b);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0444,  0x31);
    dprintk("-------------------------------wb_auto");
}

void gt2005_set_wb_cloud(struct i2c_client *client)
{
    dprintk("-------------------------------wb_cloud");
    //gt2005_write_reg16(client,0x031A , 0x81);
    gt2005_write_reg16(client,0x0320,  0x02);
    gt2005_write_reg16(client,0x0321,  0x02);
    gt2005_write_reg16(client,0x0322,  0x02);
    gt2005_write_reg16(client,0x0323 , 0x02);
    gt2005_write_reg16(client,0x0441,  0x80);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0444,  0x00);
}

void gt2005_set_wb_daylight(struct i2c_client *client)
{
    dprintk("-------------------------------wb_daylight");
    gt2005_write_reg16(client,0x0320,  0x02);
    gt2005_write_reg16(client,0x0321,  0x02);
    gt2005_write_reg16(client,0x0322,  0x02);
    gt2005_write_reg16(client,0x0323 , 0x02);
    gt2005_write_reg16(client,0x0441,  0x60);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0444,  0x14);
}

void gt2005_set_wb_incandescence(struct i2c_client *client)
{
    dprintk("-------------------------------wb_incandescence");                               
    gt2005_write_reg16(client,0x0320,  0x02);
    gt2005_write_reg16(client,0x0321,  0x02);
    gt2005_write_reg16(client,0x0322,  0x02);
    gt2005_write_reg16(client,0x0323 , 0x02);
    gt2005_write_reg16(client,0x0441,  0x50);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0444,  0x30);
}

void gt2005_set_wb_fluorescent(struct i2c_client *client)
{
    dprintk("-------------------------------wb_fluorescent");                        
    gt2005_write_reg16(client,0x0320,  0x02);
    gt2005_write_reg16(client,0x0321,  0x02);
    gt2005_write_reg16(client,0x0322,  0x02);
    gt2005_write_reg16(client,0x0323 , 0x02);
    gt2005_write_reg16(client,0x0441,  0x43);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0444,  0x4B);
}

void gt2005_set_wb_tungsten(struct i2c_client *client)
{
    struct gt2005_sensor *s;

    gt2005_write_reg16(client,0x0320,  0x02);
    gt2005_write_reg16(client,0x0321,  0x02);
    gt2005_write_reg16(client,0x0322,  0x02);
    gt2005_write_reg16(client,0x0323 , 0x02);
    gt2005_write_reg16(client,0x0441,  0x0b);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0442,  0x00);
    gt2005_write_reg16(client,0x0444,  0x5e);
}

int gt2005_set_balance(struct cim_sensor * sensor_info,unsigned short arg)
{
	struct gt2005_sensor *s;
	struct i2c_client * client ;
	unsigned char *str_balance;
	s = container_of(sensor_info, struct gt2005_sensor, cs);
	client = s->client;
	
	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			gt2005_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
		case WHITE_BALANCE_INCANDESCENT :
			gt2005_set_wb_incandescence(client);
			str_balance = "WHITE_BALANCE_INCANDESCENT";
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			gt2005_set_wb_fluorescent(client);
			str_balance = "WHITE_BALANCE_FLUORESCENT";
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			str_balance = "WHITE_BALANCE_WARM_FLUORESCENT";
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			gt2005_set_wb_daylight(client);
			str_balance = "WHITE_BALANCE_DAYLIGHT";
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			gt2005_set_wb_cloud(client);
			str_balance = "WHITE_BALANCE_CLOUDY_DAYLIGHT";
			break;
		case WHITE_BALANCE_TWILIGHT :
			str_balance = "WHITE_BALANCE_TWILIGHT";
			break;
		case WHITE_BALANCE_SHADE :
			str_balance = "WHITE_BALANCE_SHADE";
			break;
		default:
			gt2005_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
	}
	return 0;	
}
