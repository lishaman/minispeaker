/*
*
*xpdong@ingenic.cn
*i2c addr:0x46
*/

#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/tsc.h>

#include "ctp_it7260.h"

static struct i2c_client *Ctp_it7260_client;

#define DEBUGMODE 0
#if DEBUGMODE
#define printk(x...) printk(x)
#else
#define printk(x...) 
#endif

#define IT7260_RETRY_COUNT                              1
#define MAX_FINGER_NUM					3
#define COMMAND_BUFFER_INDEX				0x20
#define QUERY_BUFFER_INDEX				0x80
#define COMMAND_RESPONSE_BUFFER_INDEX			0xA0
#define POINT_BUFFER_INDEX				0xE0
#define QUERY_SUCCESS					0x00
#define QUERY_BUSY					0x01
#define QUERY_ERROR					0x02
#define QUERY_POINT					0x80

//xzr 
#include <linux/syscalls.h>
#include <linux/module.h>  // Needed by all modules
#include <linux/kernel.h>  // Needed for KERN_INFO
#include <linux/fs.h>      // Needed by filp
#include <asm/uaccess.h>   // Needed by segment descriptors
#define MAX_BUFFER_SIZE  255
#define SIGNATURE_LENGTH 16
bool fnSetStartOffset(int wOffset);
bool fnAdvanceWriteFlash(int unLength, char* pData);
 static int  Ctp_it7260_probe(struct i2c_client *client, const struct i2c_device_id *id);
//xzr 
static struct ctp_it7260_platform_data *ctp_it7260_pdata;
static u8 gpucPointBuffer[14];
static u32 gpdwSampleX[3],gpdwSampleY[3],gpdwPressure[1];
static int temp_finger = 0;
static int Calibration__success_flag = 0;
static int Upgrade__success_flag = 0;

static int it7260_i2c_rxdata(struct i2c_client *client,char *reg, char *rxdata, int length)
{
	unsigned short nostart_flag = 0;
	struct i2c_msg msgs[2];

	if (!ctp_it7260_pdata->is_i2c_gpio)
		nostart_flag |= I2C_M_NOSTART;

	msgs[0].addr	= client->addr;
	msgs[0].flags	= 0 | nostart_flag;
	msgs[0].len	= 1;
	msgs[0].buf	= reg;

	msgs[1].addr	= client->addr;
	msgs[1].flags	= I2C_M_RD | nostart_flag;
	msgs[1].len	= length;
	msgs[1].buf	= rxdata;

	if (i2c_transfer(client->adapter, msgs, 2) > 0) {
		/* transfer success */
		return 0;
	} else  /* transfer fail */
		return -EIO;

	/* do not need to retry, will fail all the same */
	return 0;
}

static int it7260_i2c_txdata(struct i2c_client *client, char *txdata, int length)
{
	struct i2c_msg msg;

	msg.addr	= client->addr;
	msg.flags	= 0;
	msg.len	= length;
	msg.buf	= txdata;

	if ( i2c_transfer(client->adapter, &msg, 1) > 0) {
		/* transfer success */
		return 0;
	} else  /* transfer fail */
		return -EIO;

	/* do not need to retry, will fail all the same */
	return 0;
}

static int Ctp_it7260_rx_data(struct i2c_client *client, u8 reg,u8* rxData, int length)
{
	int ret;

	ret = it7260_i2c_rxdata(client, &reg, rxData, length);
	if (ret < 0)
		return 0;
	else
		return 1;
}

static int Ctp_it7260_tx_data(struct i2c_client *client, u8 reg,char *txData, int length)
{
	char buf[80];
	int ret;

	buf[0] = reg;
	memcpy(buf+1, txData, length);

	ret = it7260_i2c_txdata(client, buf, length+1);

	if (ret < 0)
		return 0;
	else {
		return 1;
	}
}

static int  ReadQueryBuffer(struct i2c_client *client, u8* pucData)
{
	return Ctp_it7260_rx_data(client, QUERY_BUFFER_INDEX, pucData, 1);
}

static int ReadCommandResponseBuffer(struct i2c_client *client, u8* pucData, unsigned int unDataLength)
{
	return Ctp_it7260_rx_data(client, COMMAND_RESPONSE_BUFFER_INDEX, pucData, unDataLength);
}

static int ReadPointBuffer(struct i2c_client *client, u8* pucData)
{
	return Ctp_it7260_rx_data(client, POINT_BUFFER_INDEX, pucData, 14);
}

static int WriteCommandBuffer(struct i2c_client *client, u8* pucData, unsigned int unDataLength)
{
	return Ctp_it7260_tx_data(client, COMMAND_BUFFER_INDEX, pucData, unDataLength);
}

static int Ctp_it7260_touch_open(struct input_dev *idev)
{
	return 0;
}

static void Ctp_it7260_touch_close(struct input_dev *idev)
{
	return;
}

static irqreturn_t Ctp_it7260_touch_irq(int irq, void *dev_id)
{	
	struct Ctp_it7260_data *Ctp_it7260 = dev_id;
	
	disable_irq_nosync(irq);
	queue_work(Ctp_it7260->ctp_workq, &Ctp_it7260->work);		
	return IRQ_HANDLED; 		
}

static int ts_input_init(struct i2c_client *client)
{
	int ret = -1;
	struct Ctp_it7260_data *Ctp_it7260;
	Ctp_it7260 = i2c_get_clientdata(client);

	/* register input device */
	Ctp_it7260->input_dev1 = input_allocate_device();
	if (Ctp_it7260->input_dev1 == NULL) {
		pr_emerg( "%s: failed to allocate input dev1\n",__FUNCTION__);
		return -ENOMEM;
	}
	
	/* register input device */
	Ctp_it7260->input_dev = input_allocate_device();
	if (Ctp_it7260->input_dev == NULL) {
		pr_emerg( "%s: failed to allocate input dev\n",__FUNCTION__);
		return -ENOMEM;
	}

	Ctp_it7260->input_dev->name = "CTS_Ctp_it7260";
	Ctp_it7260->input_dev1->name = "CTS_Ctp_it7260_key";
	set_bit(EV_KEY, Ctp_it7260->input_dev1->evbit);
	set_bit(KEY_MENU, Ctp_it7260->input_dev1->keybit);
	set_bit(KEY_HOME, Ctp_it7260->input_dev1->keybit);
	set_bit(KEY_BACK, Ctp_it7260->input_dev1->keybit);
	set_bit(KEY_SEARCH, Ctp_it7260->input_dev1->keybit);
	ret = input_register_device(Ctp_it7260->input_dev1);
	if (ret) {
		pr_emerg("%s: unabled to register input device, ret = %d\n",__FUNCTION__, ret);
		return ret;
	}
	set_bit(INPUT_PROP_DIRECT, Ctp_it7260->input_dev->propbit);
	Ctp_it7260->input_dev->phys = "CTS_Ctp_it7260/input1";
	Ctp_it7260->input_dev->dev.parent = &client->dev;
	Ctp_it7260->input_dev->open = Ctp_it7260_touch_open;
	Ctp_it7260->input_dev->close = Ctp_it7260_touch_close;

	set_bit(ABS_MT_POSITION_X, Ctp_it7260->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, Ctp_it7260->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, Ctp_it7260->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, Ctp_it7260->input_dev->absbit);

	//__set_bit(EV_ABS, Ctp_it7260->input_dev->evbit);
	//__set_bit(EV_KEY, Ctp_it7260->input_dev->evbit);
	set_bit(EV_ABS, Ctp_it7260->input_dev->evbit);
	set_bit(EV_KEY, Ctp_it7260->input_dev->evbit);
	
	set_bit(EV_ABS, Ctp_it7260->input_dev1->evbit);
	set_bit(EV_KEY, Ctp_it7260->input_dev1->evbit);
#ifdef CONFIG_TOUCHSCREEN_XY_YX
	input_set_abs_params(Ctp_it7260->input_dev, ABS_MT_POSITION_Y, 0, ctp_it7260_pdata->width, 0, 0);
	input_set_abs_params(Ctp_it7260->input_dev, ABS_MT_POSITION_X, 0, ctp_it7260_pdata->height, 0, 0);
#else
	input_set_abs_params(Ctp_it7260->input_dev, ABS_MT_POSITION_X, 0, ctp_it7260_pdata->width, 0, 0);
	input_set_abs_params(Ctp_it7260->input_dev, ABS_MT_POSITION_Y, 0, ctp_it7260_pdata->height, 0, 0);
#endif
	input_set_abs_params(Ctp_it7260->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);	
	input_set_abs_params(Ctp_it7260->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);

	ret = input_register_device(Ctp_it7260->input_dev);
	if (ret) {
		pr_emerg("%s: unabled to register input device, ret = %d\n",__FUNCTION__, ret);
		return ret;
	}
	return 0;
}
#if 0
static int ExitFirmwareUpgradeMode(struct i2c_client *client) 
{
	u8 ucQueryResponse;
	u8 pucCommandBuffer[128];

	do
	{
		ReadQueryBuffer(client, &ucQueryResponse);
	}
	while(ucQueryResponse & QUERY_BUSY);

	pucCommandBuffer[0] = 0x60; 
	pucCommandBuffer[1] = 0x80; 
	pucCommandBuffer[2] = 'I'; 
	pucCommandBuffer[3] = 'T'; 
	pucCommandBuffer[4] = '7'; 
	pucCommandBuffer[5] = '2'; 
	pucCommandBuffer[6] = '6'; 
	pucCommandBuffer[7] = '0'; 
	pucCommandBuffer[8] = 0xAA; 
	pucCommandBuffer[9] = 0x55; 
	if(!WriteCommandBuffer(client, pucCommandBuffer,10))
	{
		return -1;
	}

	do
	{
		ReadQueryBuffer(client, &ucQueryResponse);
	}
	while(ucQueryResponse & QUERY_BUSY);


	if(!ReadCommandResponseBuffer(client, pucCommandBuffer,1))
	{
		return -1;
	}

	if(pucCommandBuffer[0] == 0 )
	{
		return 0;
	}

	return -1;
}
#endif 
static bool Ctp_it7260_quit_update_mode(struct Ctp_it7260_data *ts)
{
	set_pin_status(ts->gpio.power, 0);
	set_pin_status(ts->gpio.irq, 0);
	msleep(20);
	set_pin_status(ts->gpio.power, 1);	  
	msleep(20);

	//ExitFirmwareUpgradeMode(client);
	set_pin_status(ts->gpio.irq, 1);

	return true;
}

static bool Ctp_it7260_init_irq(struct i2c_client *client)
{
	struct Ctp_it7260_data *Ctp_it7260;
	int ret;

	Ctp_it7260 = i2c_get_clientdata(client);

	Ctp_it7260_quit_update_mode(Ctp_it7260);

	client->irq = gpio_to_irq(Ctp_it7260->gpio.irq->num);	
	ret = request_irq(client->irq, Ctp_it7260_touch_irq,
			   IRQF_TRIGGER_LOW | IRQF_DISABLED,
			   client->name, Ctp_it7260);

	if (ret ) {
		printk(KERN_ERR "Ctp_it7260_init_irq: request irq failed with %d\n", ret);
		return ret;
	}	
	
	//disable_irq(client->irq);
	return true;	
}
#if 0
static bool ReInitFirmware(struct i2c_client *client)
{
	u8 ucWriteLength;
	u8 pucData[1];
	u8 ucQuery;

	ucWriteLength = 1;
	pucData[0] = 0x0c;

	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	} while(ucQuery & QUERY_BUSY);

	if(!WriteCommandBuffer(client, pucData, ucWriteLength))
	{
		printk("%s:failed\n",__func__);
		return false;
	}

	return true;
}
#endif

#if 0
// ================================================================================
// Function Name --- GetFirmwareInformation
// Description --- Get firmware information
// Input --- NULL
//Output --- return true if the command execute successfully, otherwuse return false.
// ================================================================================
static bool GetFirmwareInformation(struct i2c_client *client)
{
	u8 ucWriteLength, ucReadLength;
	u8 pucData[128];
	u8 ucQuery;
	int i;
	ucWriteLength = 2;
	ucReadLength = 0x09;
	pucData[0] = 0x01;
	pucData[1] = 0x00;

	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	} while(ucQuery & QUERY_BUSY);

	if(!WriteCommandBuffer(client, pucData, ucWriteLength))
	{
		return false;
	}

	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	} while(ucQuery & QUERY_BUSY);
	pucData[5]= 0 ;
	pucData[6]= 0 ;
	pucData[7]= 0 ;
	pucData[8]= 0;

	if(!ReadCommandResponseBuffer(client, pucData, ucReadLength))
	{
		return false;
	}
	
	for (i =0;i<ucReadLength;i++)
		printk("GetFirmwareInformation pucData[%d]=%d \r\n",i,pucData[i]);

	if(pucData[5]== 0 
	&& pucData[6] == 0 
	&& pucData[7] == 0 
	&& pucData[8] == 0) 
	{
		// There is no flash code,update mode
		printk("There is no flash code \r\n");
		return false;
	}

	return true;
}
static bool GetInterruptNotification(struct i2c_client *client)
{
	u8 ucWriteLength, ucReadLength;
	u8 pucData[128];
	u8 ucQuery;
	int i;
	ucWriteLength = 2;
	ucReadLength = 2;
	pucData[0] = 0x01;
	pucData[1] = 0x04;

	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	} while(ucQuery & QUERY_BUSY);

	
	if(!WriteCommandBuffer(client, pucData, ucWriteLength))
	{
		printk("GetInterruptNotification WriteCommandBuffer err!\r\n");
		return false;
	}
	
	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	} while(ucQuery & QUERY_BUSY);
	
	if(!ReadCommandResponseBuffer(client, pucData, ucReadLength))
	{
		printk("GetInterruptNotification ReadCommandResponseBuffer err!\r\n");
		return false;
	}
	
	for (i =0;i<ucReadLength;i++)
		printk("GetInterruptNotification pucData[%d]=%d \r\n",i,pucData[i]);

	

	return true;
}
#endif
#if 0
// ================================================================================
// Function Name --- SetInterruptNotification
// Description --- Set It7260 interrupt mode
// Input --- 
//	ucStatus- the interrupt status
//	ucType- the interrupt type
//Output --- return true if the command execute successfully, otherwuse return false.
// ================================================================================
static bool SetInterruptNotification(struct i2c_client *client, u8 ucStatus, u8 ucType)
{
	u8 ucWriteLength, ucReadLength;
	u8 pucData[128];
	u8 ucQuery;
	int i;

	ucWriteLength = 4;
	ucReadLength = 1;
	pucData[0] = 0x02;
	pucData[1] = 0x04;
	pucData[2] = ucStatus;
	pucData[3] = ucType;

	// Query
	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	// Write Command
	if(!WriteCommandBuffer(client, pucData, ucWriteLength))
	{
		return false;
	}

	// Query
	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	// Read Command Response
	if(!ReadCommandResponseBuffer(client, pucData, ucReadLength))
	{
		return false;
	}

	for (i =0;i<ucReadLength;i++)
		printk("SetInterruptNotification pucData[%d]=0x%x \r\n",i,pucData[i]);
	
	GetInterruptNotification(client);
	if(pucData[ucReadLength-1]) 
	{
		return false;
	}
	
	return true;
}
#endif

// ================================================================================
// Function Name --- IdentifyCapSensor
// Description --- Identify Capacitive Sensor information
// Input --- NULL
//Output --- return true if the command execute successfully, otherwuse return false.
// ================================================================================
bool IdentifyCapSensor(struct i2c_client *client)
{
	u8 ucWriteLength, ucReadLength;
	u8 pucData[128];
	u8 ucQuery=0;int i;

	ucWriteLength = 1;
	ucReadLength = 0x0A;
	pucData[0] = 0x00;
	
	printk("%s\r\n",__FUNCTION__);
	// Query
	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
		
			printk("first wait \r\n");
			//means we use resister touchscreen
			goto error_out;
			ucQuery = QUERY_BUSY;
		}
		printk("%s ucQuery = 0x%x \r\n",__FUNCTION__,ucQuery);

		if (0xff == ucQuery)
			goto error_out;
		mdelay(500);
	}while(ucQuery & QUERY_BUSY);


	// Write Command
	pucData[0] = 0x00;ucWriteLength = 1;
	if(!WriteCommandBuffer(client, pucData, ucWriteLength))
	{
		printk("WriteCommandBuffer false \r\n");
	}

	// Query
	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			printk("second wait \r\n");
			ucQuery = QUERY_BUSY;
		}
		printk("%s ucQuery1=0x%x \r\n",__FUNCTION__,ucQuery);
	} while(ucQuery & QUERY_BUSY);

	if(!ReadCommandResponseBuffer(client, pucData, ucReadLength))
	{
		printk("ReadCommandResponseBuffer false \r\n");
	}

	for (i=0;i<ucReadLength;i++)
		printk("pucData[%d]=%c \r\n",i,pucData[i]);

	return true;

error_out:
	return false;
}

// ================================================================================
// Function Name --- Get2DResolutions
// Description --- Get the resolution of X and Y axes
// Input --- 
//	pwXResolution - the X resolution
//	pwYResolution - the Y resolution
//	pucStep - the step
//Output --- return true if the command execute successfully, otherwuse return false.
// ================================================================================
bool Get2DResolutions(struct i2c_client *client, u32 *pwXResolution, u32*pwYResolution, u8 *pucStep)
{
	u8 ucWriteLength, ucReadLength;
	u8 pucData[128];
	u8 ucQuery;
	int i;

	ucWriteLength = 3;
	ucReadLength = 0x07;
	pucData[0] = 0x01;
	pucData[1] = 0x02;
	pucData[2] = 0x00;

	// Query
	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	} while(ucQuery & QUERY_BUSY);

	// Write Command
	printk("%s WriteCommandBuffer\r\n",__FUNCTION__);
	if(!WriteCommandBuffer(client, pucData, ucWriteLength))
	{
		return false;
	}

	// Query
	do
	{
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	} while(ucQuery & QUERY_BUSY);
	printk("%s ReadCommandResponseBuffer\r\n",__FUNCTION__);
	// Read Command Response
	if(!ReadCommandResponseBuffer(client, pucData, ucReadLength))
	{
		return false;
	}
	printk("%s ReadCommandResponseBuffer EDN\r\n",__FUNCTION__);

	for (i=0;i<ucReadLength;i++)
		printk("pucData[%d] = 0x%x \r\n",i,pucData[i]);
 
	if(pwXResolution != NULL) 
	{
		*pwXResolution = (pucData[2] | (pucData[3] << 8));
	}
	if(pwYResolution!= NULL) 
	{
		* pwYResolution = (pucData[4] | (pucData[5] << 8));
	}
	if(pucStep!= NULL) 
	{
		* pucStep = pucData[6];
	}
	printk("%s x res=%d y res=%d !\r\n",__FUNCTION__,*pwXResolution,* pwYResolution);
	return true;
}

// *******************************************************************************************
// Function Name: CaptouchMode
// Description: 
//   This function is mainly used to initialize cap-touch controller to active state.
// Parameters: 
//   dwMode -- the power state to be entered
// Return value: 
//   return zero if success, otherwise return non zero value
// *******************************************************************************************
int CaptouchMode(struct i2c_client *client, u8 dwMode)
{
	u8 ucQueryResponse;
	u8 pucCommandBuffer[128];

	do
	{
		ReadQueryBuffer(client, &ucQueryResponse);
	}
	while(ucQueryResponse & QUERY_BUSY);

	pucCommandBuffer[0] = 0x04;
	pucCommandBuffer[1] = 0x00;
	switch(dwMode)
	{
		case 0x00:
			pucCommandBuffer[2] = 0x00;
			break;
		case 0x01:
			pucCommandBuffer[2] = 0x01;
			break;
		case 0x02:
			pucCommandBuffer[2] = 0x02;
			break;
		default:
			return -1;
	}

	if(!WriteCommandBuffer(client, pucCommandBuffer,3))
	{
		return -1;
	}

	return 0;
}

// *******************************************************************************************
// Function Name: CaptouchReset
// Description: 
//   This function is mainly used to reset cap-touch controller by sending reset command.
// Parameters: NULL
// Return value: 
//   return TRUE if success, otherwise return FALSE
// *******************************************************************************************
static int CaptouchReset(struct i2c_client *client)
{
	u8 ucQueryResponse;
	u8 pucCommandBuffer[128];
	pucCommandBuffer[0] = 0x6F;

	do
	{
		ReadQueryBuffer(client, &ucQueryResponse);
	}
	while(ucQueryResponse & QUERY_BUSY);

	if(!WriteCommandBuffer(client, pucCommandBuffer,1))
	{
		return -1;
	}
	mdelay(200);

	do
	{
		ReadQueryBuffer(client, &ucQueryResponse);
	}
	while(ucQueryResponse & QUERY_BUSY);


	if(!ReadCommandResponseBuffer(client, pucCommandBuffer,2))
	{
		return -1;
	}

	if(pucCommandBuffer[0] == 0 && pucCommandBuffer[1] == 0)
	{
		return 0;
	}

	return -1;
}



// *******************************************************************************************
// Function Name: CaptouchHWInitial
// Description: 
//   This function is mainly used to initialize cap-touch controller to active state.
// Parameters: NULL
// Return value: 
//   return zero if success, otherwise return non zero value
// *******************************************************************************************
static int CaptouchHWInitial(struct i2c_client *client)
{
#if 0
	u32 wXResolution=0,wYResolution=0;
	u8 ucStep=0;
#endif
	
	if (!IdentifyCapSensor(client))
	{
		printk("%s IdentifyCapSensor error \r\n",__FUNCTION__);
		return false;
	}
#if 0  //¿ÉÂÔ¹ý
	if (!GetFirmwareInformation (client))
	{

		printk("%s GetFirmwareInformation error \r\n",__FUNCTION__);
	}

	if (!Get2DResolutions(client, &wXResolution, &wYResolution, &ucStep))
	{
		printk("%s Get2DResolutions error \r\n",__FUNCTION__);
	}
	if (!SetInterruptNotification(client,1,0))
	{
		printk("%s SetInterruptNotification error \r\n",__FUNCTION__);
	}

#endif
	return true;
}


// *******************************************************************************************
// Function Name: CaptouchGetSampleValue
// Description: 
//   This function is mainly used to get sample values which shall contain x, y position and pressure. 
// Parameters:
//   pdwSampleX -- the pointer of returned X coordinates
//   pdwSampleY -- the pointer of returned Y coordinates
//   pdwPressure -- the pointer of returned pressures
// Return value: 
//   The return value is the number of sample points. Return 0 if failing to get sample. The maximum //   return value is 10 in normal case. If the CTP controller does not support pressure measurement, //   the return value is the sample values OR with PRESSURE_INVALID.
// *******************************************************************************************
static int CaptouchGetSampleValue(u32* pdwSampleX, u32* pdwSampleY, u32* pdwPressure)
{
	int nRet;
	int i;

	if(gpucPointBuffer[1] & 0x01)
	{
		return MAX_FINGER_NUM + 1;
	}

	nRet = 0;
	for(i = 0; i < MAX_FINGER_NUM; i++)
	{
		if(gpucPointBuffer[0] & (1 << i))
		{
			nRet++;
			pdwSampleX[i] = ((u32)(gpucPointBuffer[i * 4 + 3] & 0x0F) << 8) | gpucPointBuffer[i * 4 + 2];
			pdwSampleY[i] = ((u32)(gpucPointBuffer[i * 4 + 3] & 0xF0) << 4) | gpucPointBuffer[i * 4 + 4];
			pdwPressure[i] = (u32)(gpucPointBuffer[i * 4 + 5] & 0x0F);

		}
		else
		{
			pdwSampleX[i] = 0;
			pdwSampleY[i] = 0;
			pdwPressure[i] = 0;
		}
	}

	return nRet;
}

// *******************************************************************************************
// Function Name: CaptouchGetGesture
// Description: 
//   This function is mainly used to initialize cap-touch controller to active state.
// Parameters: NULL
// Return value: 
//   return gesture ID
// *******************************************************************************************
static int CaptouchGetGesture(void)
{
	return (int)gpucPointBuffer[1];
}

static void  Ctp_it7260_work_func(struct work_struct  *work)
{
       	u8 ucQueryResponse;
	u32 dwTouchEvent;
	struct Ctp_it7260_data *Ctp_it7260 = container_of(work, struct Ctp_it7260_data, work);	
	struct i2c_client *client = Ctp_it7260->client;
	
	int PE1status = 0;
	int xx= 0;
	int yy= 0;
	
	PE1status = get_pin_status(Ctp_it7260->gpio.irq);
	if (!PE1status)
	{
		if(!ReadQueryBuffer(client, &ucQueryResponse))
		{
			printk("ReadQueryBuffer exit because of i2c error\n");
			ReadPointBuffer(client, gpucPointBuffer);
			enable_irq(Ctp_it7260->client->irq);			
			return;
		}
		printk("%s++++ %d  ucQueryResponse=0x%x \r\n",__FUNCTION__,__LINE__,ucQueryResponse);
		// Touch Event
		if(ucQueryResponse & QUERY_POINT)
		{
			if(!ReadPointBuffer(client, gpucPointBuffer))
			{
				printk("ReadPointBuffer exit because of i2c error\n");
				enable_irq(Ctp_it7260->client->irq);
				return;
			}
			switch(gpucPointBuffer[0] & 0xF0)
			{
				case 0x00:
					dwTouchEvent = CaptouchGetSampleValue(gpdwSampleX, gpdwSampleY, gpdwPressure);
					if(dwTouchEvent == 0)
					{
						printk("TOUCH Release  !!\r\n");
						temp_finger = 0;
						//input_report_abs(Ctp_it7260->input_dev, ABS_MT_TOUCH_MAJOR, 0);
						input_mt_sync(Ctp_it7260->input_dev);  //modify for 4.0 MutiTouch
						input_sync(Ctp_it7260->input_dev);
					}
					else if(dwTouchEvent <=MAX_FINGER_NUM) //CTP_MAX_FINGER_NUMBER)
					{
						if(dwTouchEvent == 1)
						{
							if (gpucPointBuffer[0] & 0x01) 
							{
								/*if the point exceed screen size, we just drop it*/
								if (gpdwSampleX[0] <= ctp_it7260_pdata->width &&
								    gpdwSampleY[0] <= ctp_it7260_pdata->height)
								{
                            		if (temp_finger == 2)
									{
										//input_report_abs(Ctp_it7260->input_dev, ABS_MT_TOUCH_MAJOR, 0);
										input_mt_sync(Ctp_it7260->input_dev);	
										temp_finger = 1;
									}

#ifdef CONFIG_TOUCHSCREEN_XY_YX
                                    xx = gpdwSampleY[0];
                                    yy = ctp_it7260_pdata->width - gpdwSampleX[0];
#else
                                    xx = gpdwSampleX[0];
                                    yy = gpdwSampleY[0];
#endif
				    
									input_report_abs(Ctp_it7260->input_dev, ABS_MT_TOUCH_MAJOR, gpdwPressure[0]);
									input_report_abs(Ctp_it7260->input_dev, ABS_MT_POSITION_X,xx);
									input_report_abs(Ctp_it7260->input_dev, ABS_MT_POSITION_Y,yy);
									input_report_abs(Ctp_it7260->input_dev, ABS_MT_WIDTH_MAJOR, 1);
									input_mt_sync(Ctp_it7260->input_dev);	
									printk("--1--dwTouchEvent == 0 x=%d  y=%d \r\n",xx,yy);
							
								}
								else
									printk("drop a illeage point!\n");

							}
							else if (gpucPointBuffer[0] & 0x02) 
							{

                                if (gpdwSampleX[1] <= ctp_it7260_pdata->width &&
                                        gpdwSampleY[1] <= ctp_it7260_pdata->height)
                                {
                                    if (temp_finger == 2) {
					    //input_report_abs(Ctp_it7260->input_dev, ABS_MT_TOUCH_MAJOR, 0);
                                        input_mt_sync(Ctp_it7260->input_dev);	
                                        temp_finger = 1;
                                    }

#ifdef CONFIG_TOUCHSCREEN_XY_YX
                                    xx = gpdwSampleY[1];
                                    yy = ctp_it7260_pdata->width - gpdwSampleX[1];

#else
                                    xx = gpdwSampleX[1];
                                    yy = gpdwSampleY[1];
#endif
                                    input_report_abs(Ctp_it7260->input_dev, ABS_MT_TOUCH_MAJOR, gpdwPressure[1]);
                                    input_report_abs(Ctp_it7260->input_dev, ABS_MT_POSITION_X, xx);
                                    input_report_abs(Ctp_it7260->input_dev, ABS_MT_POSITION_Y, yy);
                                    input_report_abs(Ctp_it7260->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                                    input_mt_sync(Ctp_it7260->input_dev);	

                                    printk("-1-dwTouchEvent == 1 x=%d  y=%d \r\n",xx,yy);

                                }
								else
									printk("drop a illeage point!\n");
                            }
                            
                            input_sync(Ctp_it7260->input_dev);
                        } 
						else if (dwTouchEvent == 2) 
						{
							printk("%s: two fingers touched!!!!!\n", __FUNCTION__);
							printk("-2-x0: %d, y0: %d, p0: %d |||| x1:%d, y1:%d, p1:%d\n", gpdwSampleX[0],gpdwSampleY[0],gpdwPressure[0],gpdwSampleX[1],gpdwSampleY[1],gpdwPressure[1]);
							temp_finger = 2;
                            
#ifdef CONFIG_TOUCHSCREEN_XY_YX
                            xx = gpdwSampleY[0];
                            yy = ctp_it7260_pdata->width - gpdwSampleX[0];
#else
                            xx = gpdwSampleX[0];
                            yy = gpdwSampleY[0];
#endif

							input_report_abs(Ctp_it7260->input_dev, ABS_MT_TOUCH_MAJOR, gpdwPressure[0]);
							input_report_abs(Ctp_it7260->input_dev, ABS_MT_POSITION_X, xx);
							input_report_abs(Ctp_it7260->input_dev, ABS_MT_POSITION_Y, yy);
							input_report_abs(Ctp_it7260->input_dev, ABS_MT_WIDTH_MAJOR, 1);
							input_mt_sync(Ctp_it7260->input_dev);	
							
#ifdef CONFIG_TOUCHSCREEN_XY_YX
                                    xx = gpdwSampleY[1];
                                    yy = ctp_it7260_pdata->width - gpdwSampleX[1];

#else
                                    xx = gpdwSampleX[1];
                                    yy = gpdwSampleY[1];
#endif
							input_report_abs(Ctp_it7260->input_dev, ABS_MT_TOUCH_MAJOR, gpdwPressure[0]);
							input_report_abs(Ctp_it7260->input_dev, ABS_MT_POSITION_X, xx);
							input_report_abs(Ctp_it7260->input_dev, ABS_MT_POSITION_Y, yy);
							input_report_abs(Ctp_it7260->input_dev, ABS_MT_WIDTH_MAJOR, 1);
							input_mt_sync(Ctp_it7260->input_dev);	
							
                            input_sync(Ctp_it7260->input_dev);
						}
						else
							printk("the same \r\n");
					}
					else
					{
						printk("%s SYSTEM_TOUCH_EVENT_PALM_DETECT \r\n",__FUNCTION__);
					}
					break;
				case 0x80:
					dwTouchEvent = CaptouchGetGesture();
					printk("gesture:0x%x \r\n",dwTouchEvent);
					break;
				//for touchkey
				case 0x40:
					if (0 == ((gpucPointBuffer[0] & 0x40) && (gpucPointBuffer[0] & 0x01)) )
					{
						break;
					}
					if (gpucPointBuffer[2]) 
					{//Button down
						switch (gpucPointBuffer[1]) 
						{
							case 1:
							{
								input_report_key(Ctp_it7260->input_dev1, KEY_MENU, 1) ;
								input_sync(Ctp_it7260->input_dev1) ;
								//printk("TP260 Robert Menu\n");
							}
							//"Menu" button is down
							break;

							case 2:
							{
								input_report_key(Ctp_it7260->input_dev1, KEY_HOME, 1) ;
								input_sync(Ctp_it7260->input_dev1) ;
								//printk("...TP260 Robert Home\n");
							}
							//"Home" button is down
							break;

							case 3:
							{ 
								input_report_key(Ctp_it7260->input_dev1, KEY_SEARCH, 1) ;
								input_sync(Ctp_it7260->input_dev1) ;
								//printk("...TP260 Robert Search\n");
							}
							//"Search" button is down
							break;
							case 4:
							{
								input_report_key(Ctp_it7260->input_dev1, KEY_BACK, 1) ;
								input_sync(Ctp_it7260->input_dev1) ;
								//printk("...TP260 Robert down\n");
							}
							//"Back" button is down
							break;
						}
					} 
					else 
					{//Button up
						switch (gpucPointBuffer[1]) 
						{
							case 1:
								input_report_key(Ctp_it7260->input_dev1, KEY_MENU, 0) ;
								input_sync(Ctp_it7260->input_dev1) ;
								//printk("...TP260 Robert Menu\n");
								//"Menu" button is up
								break;
							case 2:
								input_report_key(Ctp_it7260->input_dev1, KEY_HOME, 0) ;
								input_sync(Ctp_it7260->input_dev1) ;
								//printk("...TP260 Robert Home\n");
								//"Home" button is up
								break;
							case 3:
								input_report_key(Ctp_it7260->input_dev1, KEY_SEARCH, 0) ;
								input_sync(Ctp_it7260->input_dev1) ;
								//printk("...TP260 Robert Search\n");
								//"Search" button is up
								break;
							case 4:
								input_report_key(Ctp_it7260->input_dev1,  KEY_BACK, 0) ;
								input_sync(Ctp_it7260->input_dev1) ;
								//printk("...TP260 Robert down\n");
								//"Back" button is up
								break;
						}
					}
								
					break;
				default:
					printk("default \r\n");
					break;
		}
	}
	else if (ucQueryResponse & QUERY_ERROR)
		{
		if (!CaptouchReset(client))
			printk("!! CaptouchReset success \r\n");
		mdelay(100);
		if (!CaptouchMode(client, 0x00))
			printk("!! CaptouchMode success \r\n");
		}
	}
	printk("intr work finish!\n");
	enable_irq(Ctp_it7260->client->irq);
}

static ssize_t Ctp_it7260_suspend_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct Ctp_it7260_data *Ctp_it7260 = (struct Ctp_it7260_data *)i2c_get_clientdata(Ctp_it7260_client);
	u8 ucWriteLength;
	u8 pucData[128];
	u8 ucQuery;

	ucWriteLength = 3;
	pucData[0] = 0x04;
	pucData[1] = 0x00;
	pucData[2] = 0x02;

	// Query
	do
	{
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	} while(ucQuery & QUERY_BUSY);

	// Write Command
	printk("%s WriteCommandBuffer\r\n",__FUNCTION__);
	if(!WriteCommandBuffer(Ctp_it7260_client, pucData, ucWriteLength))
	{
		printk("%s:error!\n",__func__);
		return false;
	}
	disable_irq(Ctp_it7260->client->irq);
	Ctp_it7260_quit_update_mode(Ctp_it7260);

	return 1;
}

static DEVICE_ATTR(suspend, 0755, NULL,Ctp_it7260_suspend_store);

static int CalibrationCapSensor(void)
{
	unsigned char ucQuery;
	unsigned char pucCmd[80];
	int ret = 0;
	int test_read_count=0;

	pr_info("=entry CalibrationCapSensor=\n");
	if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
	{
		ucQuery = 0xFF;
	}

	pr_info("=CalibrationCapSensor read 0x80 1=%d=\n",ucQuery);
	test_read_count=0;

	while((ucQuery & 0x01) && (test_read_count < 0x80000))
	{
		test_read_count++;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = 0xFF;
		}
	}

	pr_info("=CalibrationCapSensor write cmd=\n");
	memset(pucCmd,0x00,20);
	pucCmd[0] = 0x20;
	pucCmd[1] = 0x13;
	pucCmd[2] = 0x00;
	////0x13 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
	pr_info("=CalibrationCapSensor write cmd--test=\n");
	ret = it7260_i2c_txdata(Ctp_it7260_client, pucCmd, 13);
	pr_info("=CalibrationCapSensor write cmd--test end=\n");
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_write_byte_data failed\n");
		/* fail? */
		return ret;
	}

	if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
	{
		ucQuery = 0xFF;
	}
	test_read_count=0;
	pr_info("=CalibrationCapSensor read 0x80 2=%d=\n",ucQuery);
	while((ucQuery & 0x01)&&(test_read_count<0x80000))
	{
		test_read_count++;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = 0xFF;
		}
	}
	pr_info("=CalibrationCapSensor read 0x80 3=%d=\n",ucQuery);

	memset(pucCmd,0x00,20);
	ret = Ctp_it7260_rx_data(Ctp_it7260_client, 0xA0, pucCmd, 1);
	pr_info("=CalibrationCapSensor read id-1-[%d][%x]=\n",ret,pucCmd[0]);

	pr_info("=CalibrationCapSensor write end=\n");

	if (ret == 1)
		return 0;
	else
		return 1;
}

ssize_t IT7260_calibration_show_temp(char *buf)
{
	return sprintf(buf, "%d", Calibration__success_flag);
}
ssize_t IT7260_calibration_store_temp(const char *buf)
{
	if(!CalibrationCapSensor()) {
		printk("IT7260_calibration_OK\n\n");
		Calibration__success_flag = 1;
		return 0;
	} else {
		printk("IT7260_calibration_failed\n");
		Calibration__success_flag = 0;
		return -1;
	}
}

static ssize_t IT7260_calibration_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "%s():\n", __func__);
	return IT7260_calibration_show_temp(buf);
}

static ssize_t IT7260_calibration_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	printk(KERN_DEBUG "%s():\n", __func__);

	IT7260_calibration_store_temp(buf);

	return count;
}

//=========================================================================================================
//=========================================================================================================

static bool fnFirmwareReinitialize(void)
{
	char ucQuery = 0xFF;
	char pucBuffer[MAX_BUFFER_SIZE];
	short wCommandResponse = 0xFFFF;
	
	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	pucBuffer[0] = 0x6F;

	if(!WriteCommandBuffer(Ctp_it7260_client, pucBuffer, 1))
	{
		printk("fnFirmwareReinitialize 1\n");
		return false;
	}
	
	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	if(!ReadCommandResponseBuffer(Ctp_it7260_client, (u8 *)&wCommandResponse, 2 ))
	{
		printk("fnFirmwareReinitialize 2\n");
		return false;
	}
	if(wCommandResponse != 0x0000)
	{
		printk("fnFirmwareReinitialize 3_%x\n",wCommandResponse);
		return false;
	}
	return true;
}

static bool fnExitFirmwareUpgradeMode(void)
{
	char ucQuery = 0xFF;
	char pucBuffer[MAX_BUFFER_SIZE];
	short wCommandResponse = 0xFFFF;

	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	pucBuffer[0] = 0x60;
	pucBuffer[1] = 0x80;
	pucBuffer[2] = 'I';
	pucBuffer[3] = 'T';
	pucBuffer[4] = '7';
	pucBuffer[5] = '2';
	pucBuffer[6] = '6';
	pucBuffer[7] = '0';
	pucBuffer[8] = 0xAA;
	pucBuffer[9] = 0x55;

	if(!WriteCommandBuffer(Ctp_it7260_client, pucBuffer, 10)  )
	{
		printk("fnExitFirmwareUpgradeMode 1\n");
		return false;
	}

	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);


	if(!ReadCommandResponseBuffer(Ctp_it7260_client, (u8*)&wCommandResponse, 2 ))
	{
		printk("fnExitFirmwareUpgradeMode 2\n");
		return false;
	}

	if(wCommandResponse != 0x0000)
	{
		printk("fnExitFirmwareUpgradeMode 3_%x\n",wCommandResponse);
		return false;
	}

	return true;
}

static bool fnDownloadConfig(int unConfigLength, char* pConfig)
{
	int wFlashSize = 0x8000;

	if(!fnSetStartOffset(wFlashSize - unConfigLength))
	{
		printk("fnDownloadConfig 1\n");
		return false;
	}

	if(!fnAdvanceWriteFlash(unConfigLength, pConfig))
	{
		printk("fnDownloadConfig 2\n");
		return false;
	}
	
  return true;
}

bool fnWriteFlash(int unLength, char* pData)
{
	char ucQuery = 0xFF;
	char pucBuffer[MAX_BUFFER_SIZE];
	short wCommandResponse = 0xFFFF;
	int i;

	if(unLength > 128 || pData == NULL)
	{
		printk("fnWriteFlash 1\n");
		return false;
	}

	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	pucBuffer[0] = 0x62;
	pucBuffer[1] = (char)unLength;
	
	for (i = 2; i < 130; i++) {
		pucBuffer[i] = pData[i-2];
	}

	if(!WriteCommandBuffer(Ctp_it7260_client, pucBuffer, unLength + 2))
	{
		printk("fnWriteFlash 2\n");
		return false;
	}

	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	if(!ReadCommandResponseBuffer(Ctp_it7260_client, (u8*)&wCommandResponse, 2 ) )
	{
		printk("fnWriteFlash 3\n");
		return false;
	}
	if(wCommandResponse != 0x0000)
	{
		printk("fnWriteFlash 4_%x\n",wCommandResponse);
		return false;
	}
	
  return true;
}

bool fnAdvanceWriteFlash(int unLength, char* pData)
{
	int unCurRoundLength;
	int unRemainderLength = unLength;

	while(unRemainderLength > 0)
	{
		if(unRemainderLength > 128)
		{
			unCurRoundLength = 128;
		}
		else
	  {
		   unCurRoundLength = unRemainderLength;
	  }

	if(fnWriteFlash(unCurRoundLength, pData + (unLength - unRemainderLength)))
	{
		printk("fnAdvanceWriteFlash 1\n");
		return false;
	}

	unRemainderLength -= unCurRoundLength;
	}
	return true;
}

bool fnReadFlash(int unLength, char* pData)
{
	char ucQuery = 0xFF;
	char pucBuffer[MAX_BUFFER_SIZE];

	if(unLength > 128 || pData == NULL){
		printk("fnReadFlash 1\n");
		return false;
	}

	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	pucBuffer[0] = 0x63;
	pucBuffer[1] = (char)unLength;

	printk("fnReadFlash A\n");
	if(it7260_i2c_txdata(Ctp_it7260_client, pucBuffer, 2) < 0 )
	{
		printk("fnReadFlash 2\n");
		return false;
	}
	printk("fnReadFlash B\n");

	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	if( !ReadCommandResponseBuffer(Ctp_it7260_client, (u8*)&pData, unLength )  )
	{
		printk("fnReadFlash 3\n");
		return false;
	}
	return true;
}

bool fnSetStartOffset(int wOffset)
{
	char ucQuery = 0xFF;
	char pucBuffer[MAX_BUFFER_SIZE];
	short wCommandResponse = 0xFFFF;

	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);


	pucBuffer[0] = 0x61;
	pucBuffer[1] = 0;
	pucBuffer[2] = wOffset & 0xFF;
	pucBuffer[3] = ( wOffset & 0xFF00 ) >> 8;

	if( !WriteCommandBuffer(Ctp_it7260_client, pucBuffer, 4) )
	{
		printk("fnSetStartOffset 1\n");
		return false;
	}

	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	if( !ReadCommandResponseBuffer (Ctp_it7260_client, (u8*)&wCommandResponse, 2)  )
	{
		printk("fnSetStartOffset 2\n");
		return false;
	}
	
	if(wCommandResponse != 0x0000)
	{
		printk("fnSetStartOffset 3_%x\n",wCommandResponse);
		return false;
	}
		
	return true;
}

bool fnVerifyFirmware(int unFirmwareLength, char* pFirmware)
{
	int wFlashSize = 0x8000;
	int wSize = 0;
	char pucBuffer[SIGNATURE_LENGTH];

	// Check signature
	if(*(pFirmware + 0) != 'I' ||
	*(pFirmware + 1) != 'T' ||
	*(pFirmware + 2) != '7' ||
	*(pFirmware + 3) != '2' ||
	*(pFirmware + 4) != '6' ||
	*(pFirmware + 5) != '0' ||
	*(pFirmware + 6) != 'F' ||
	*(pFirmware + 7) != 'W')
	{
		printk("fnVerifyFirmware 1\n");
		return false;
	}
	
	if(!fnSetStartOffset(wFlashSize - SIGNATURE_LENGTH))
	{
		printk("fnVerifyFirmware 2\n");
		return false;
	}

	printk("fnReadFlash start\n");
	if(!fnReadFlash(SIGNATURE_LENGTH, pucBuffer))
	{
		printk("fnVerifyFirmware 3\n");
		return false;
	}

	if(pucBuffer[0] == 'C'
		&& pucBuffer[1] == 'O'
		&& pucBuffer[2] == 'N'
		&& pucBuffer[3] == 'F'
		&& pucBuffer[4] == 'I'
		&& pucBuffer[5] == 'G')
	{
		//memcpy(&wSize, pucBuffer + 6, sizeof(WORD));
		wSize |= pucBuffer[6];
		wSize |= ( pucBuffer[7] << 8 );
		if(( wSize + unFirmwareLength ) >= wFlashSize )
		{
			printk("fnVerifyFirmware 4\n");
			return false;
		}
	}

	return true;
}

bool fnDownloadFirmware(int unFirmwareLength, char* pFirmware)
{
	printk("fnDownloadFirmware %x\n",unFirmwareLength);
	if(!fnVerifyFirmware(unFirmwareLength, pFirmware))
	{
		printk("fnDownloadFirmware 1\n");
		return false;
	}

	if(!fnSetStartOffset(0))
	{
		printk("fnDownloadFirmware 2\n");
		return false;
	}

	if(!fnAdvanceWriteFlash(unFirmwareLength, pFirmware))
	{
		printk("fnDownloadFirmware 3\n");
		return false;
	}

	return true;
}

bool fnEnterFirmwareUpgradeMode(void)
{
	char ucQuery = 0xFF;
	char pucBuffer[MAX_BUFFER_SIZE];
	short wCommandResponse = 0xFFFF;

	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	} while(ucQuery & QUERY_BUSY);

	pucBuffer[0] = 0x60;
	pucBuffer[1] = 0x00;
	pucBuffer[2] = 'I';
	pucBuffer[3] = 'T';
	pucBuffer[4] = '7';
	pucBuffer[5] = '2';
	pucBuffer[6] = '6';
	pucBuffer[7] = '0';
	pucBuffer[8] = 0x55;
	pucBuffer[9] = 0xAA;

	if( !WriteCommandBuffer(Ctp_it7260_client, pucBuffer, 10) )
	{
		printk("fnEnterFirmwareUpgradeMode 1\n");
		return false;
	}
	
	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(Ctp_it7260_client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	}while(ucQuery & QUERY_BUSY);

	if(!ReadCommandResponseBuffer(Ctp_it7260_client, (u8*)&wCommandResponse, 2) ){
			printk("fnEnterFirmwareUpgradeMode 2\n");
			return false;
	}

	if(wCommandResponse != 0x0000){
		printk("fnEnterFirmwareUpgradeMode 3_%x\n", wCommandResponse); //0x805e0000
		return false;
	}
	return true;
}

bool fnFirmwareDownload(int unFirmwareLength, char* pFirmware, int unConfigLength, char* pConfig)
{
	if((unFirmwareLength == 0 || pFirmware == NULL) && (unConfigLength == 0 || pConfig == NULL))	{  
    // Input invalidate
    printk("fnFirmwareDownload 1\n");
		return false;
	}

	if(!fnEnterFirmwareUpgradeMode())	{
    printk("fnFirmwareDownload 2\n");
		return false;
	}
/*
	if(unFirmwareLength != 0 && pFirmware != NULL){
		// Download firmware
		if(!fnDownloadFirmware(unFirmwareLength, pFirmware)){
			printk("fnFirmwareDownload 3\n");
			return false;
		}
	}
*/
	if(unConfigLength != 0 && pConfig != NULL){
		// Download configuration
		if(!fnDownloadConfig(unConfigLength, pConfig)){
			printk("fnFirmwareDownload 4\n");
			return false;
		}		
	}

	if(!fnExitFirmwareUpgradeMode()){
			printk("fnFirmwareDownload 5\n");
		return false;
	}

	if(!fnFirmwareReinitialize()){
		printk("fnFirmwareDownload 6\n");
		return false;
	}
	
	return true;
}

static int Upgrade_FW_CFG(void)
{
	int fw_ret = -1;
	int config_ret = -1;
//	int test_ret = -1;
//	int test2_ret = -1;
	struct file * fw_fd = NULL;
	struct file *config_fd = NULL;
//	struct file *test_fd = NULL;
//	struct file *test2_fd = NULL;
	mm_segment_t fs;
	char *fw_buf = kzalloc(0x8000, GFP_KERNEL);
	char *config_buf = kzalloc(0x500, GFP_KERNEL);
	if ( fw_buf  == NULL || fw_buf == NULL  ){
		printk("kzalloc failed\n  ");
	}

	fs = get_fs();
	set_fs(get_ds());

	fw_fd = filp_open("/sdcard/it7260_FW", O_RDONLY, 0);
	if (fw_fd < 0)
		printk("sys_open /sdcard/it7260_FW failed\n");
	else
		//fw_ret = sys_read(fw_fd, fw_buf, 0x8000);
		fw_ret = fw_fd->f_op->read(fw_fd, fw_buf, 0x8000, &fw_fd->f_pos);
	printk("fw_ver : %x,%x,%x,%x\n",fw_buf[8], fw_buf[9], fw_buf[10], fw_buf[11]);
	printk("--------------------- fw_ret = %x\n", fw_ret);

	config_fd = filp_open("/sdcard/it7260_CFG", O_RDONLY, 0);
	if (config_fd < 0)
		printk("sys_open /sdcard/it7260_CFG failed\n");
	else
		//fw_ret = sys_read(fw_fd, fw_buf, 0x8000);
		config_ret = config_fd->f_op->read(config_fd, config_buf, 0x500, &config_fd->f_pos);
	printk("cfg_ver : %x,%x,%x,%x\n",config_buf[config_ret-8], config_buf[config_ret-7], config_buf[config_ret-6], config_buf[config_ret-5]);
	printk("--------------------- config_ret = %x\n", config_ret);

	set_fs(fs);
	filp_close(fw_fd,NULL);
	filp_close(config_fd,NULL);

//xzr 2011-11-25
#if 0
	fs = get_fs();
	set_fs(get_ds());
	test_fd = filp_open("/sdcard/test", O_RDWR, 0);
	if (test_fd == NULL) {
		printk("filp_open /sdcard/test failed\n");
	}
	test2_fd = filp_open("/sdcard/test2", O_RDWR, 0);
	if( test_fd == NULL){
		printk("filp_open /sdcard/test2 failed\n");
	}
	printk("f_count = %x\n", test2_fd->f_count);
	test2_ret = test2_fd->f_op->read(test2_fd, fw_buf, 20, &test2_fd->f_pos);
	if (test2_ret < 0)
		printk("sys_read test2 to fw_buf 3 failed\n");
	else
		printk("test2_ret = %d\n", test2_ret);
	printk("%x, %x, %x\n",fw_buf[0], fw_buf[1], fw_buf[2]);
	test2_ret = test2_fd->f_op->write(test2_fd, fw_buf, 3 ,&test2_fd->f_pos);
	if (test2_ret < 0)
		printk("sys_write fw_buf to test2 3 failed\n");
	set_fs(fs);
	filp_close(test_fd,NULL);
	filp_close(test2_fd,NULL);
	return 1;
#endif
//xzr 2011-11-25
	disable_irq(Ctp_it7260_client->irq);
	if (fnFirmwareDownload(fw_ret, fw_buf, config_ret, config_buf) == false){
		//fail
		enable_irq(Ctp_it7260_client->irq);
		return 1; 
	}else{
		//success
		enable_irq(Ctp_it7260_client->irq);
		return 0;
  }
}
ssize_t IT7260_upgrade_show_temp(char *buf)
{
	return sprintf(buf, "%d", Upgrade__success_flag);
}
ssize_t IT7260_upgrade_store_temp(void)
{
	if(!Upgrade_FW_CFG()) {
		printk("IT7260_upgrade_OK\n\n");
		Upgrade__success_flag = 1;
		return 0;
	} else {
		printk("IT7260_upgrade_failed\n");
		Upgrade__success_flag = 0;
		return -1;
	}
}

static ssize_t IT7260_upgrade_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "%s():\n", __func__);
	return IT7260_upgrade_show_temp(buf);
}

static ssize_t IT7260_upgrade_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	printk(KERN_DEBUG "%s():\n", __func__);

	IT7260_upgrade_store_temp();
	return count;
}

//=========================================================================================================
//=========================================================================================================

static int Ctp_it7260_remove(struct i2c_client *client)
{	
	return 0;
}

static int Ctp_it7260_suspend(struct i2c_client *client, pm_message_t mesg)
{
	u8 ucWriteLength;
	u8 pucData[128];
	u8 ucQuery;

	ucWriteLength = 3;
	pucData[0] = 0x04;
	pucData[1] = 0x00;
	pucData[2] = 0x02;

	do{
		ucQuery = 0xFF;
		if(!ReadQueryBuffer(client, &ucQuery))
		{
			ucQuery = QUERY_BUSY;
		}
	} while(ucQuery & QUERY_BUSY);

	if(!WriteCommandBuffer(client, pucData, ucWriteLength))
	{
		printk("%s:error!\n",__func__);
		return false;
	}

	disable_irq(client->irq);
	return 0;
}

static int Ctp_it7260_resume(struct i2c_client *client)
{
	u8 ucQuery;
	printk("%s\n",__func__);
//xzr 
	enable_irq(client->irq);
//
 	Ctp_it7260_init_irq(client);
 	//msleep_interruptible(20);
	ReadQueryBuffer(client, &ucQuery);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void Ctp_it7260_early_suspend(struct early_suspend *h)
{
	struct Ctp_it7260_data *ts;
	ts = container_of(h, struct Ctp_it7260_data, early_suspend);
	Ctp_it7260_suspend(ts->client, PMSG_SUSPEND);
}

static void Ctp_it7260_late_resume(struct early_suspend *h)
{
	struct Ctp_it7260_data *ts;
	ts = container_of(h, struct Ctp_it7260_data, early_suspend);
	Ctp_it7260_resume(ts->client);
}
#endif

static DEVICE_ATTR(calibration, 0666, IT7260_calibration_show, IT7260_calibration_store);
static DEVICE_ATTR(upgrade, 0666, IT7260_upgrade_show, IT7260_upgrade_store);

static struct attribute *Ctp_it7260_attributes[] = {
	&dev_attr_suspend.attr,
	&dev_attr_calibration.attr,
	&dev_attr_upgrade.attr,
	NULL
};

static const struct attribute_group Ctp_it7260_attr_group = {
	.attrs = Ctp_it7260_attributes,
};

static const struct i2c_device_id Ctp_it7260_id[] = {
		{"ctp_it7260", 0},
		{ }
};

static struct i2c_driver Ctp_it7260_driver = {
	.driver = {
		.name 	= "ctp_it7260",
	},
	.id_table 	= Ctp_it7260_id,
	.probe		= Ctp_it7260_probe,
	.remove    	= Ctp_it7260_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= Ctp_it7260_suspend,
	.resume		= Ctp_it7260_resume,
#endif
};

static void Ctp_it7260_gpio_init(struct Ctp_it7260_data *ts)
{
	struct device *dev = &ts->client->dev;
	struct jztsc_platform_data *pdata = dev->platform_data;

	ts->gpio.irq = &pdata->gpio[0];
	ts->gpio.power = &pdata->gpio[1];
		
	if (gpio_request_one(ts->gpio.irq->num,
			     GPIOF_DIR_IN, "ctp_it7260_irq")) {
		dev_err(dev, "no irq pin available\n");
		ts->gpio.irq->num = -EBUSY;
	}
	if (gpio_request_one(ts->gpio.power->num,
			     ts->gpio.power->enable_level
			     ? GPIOF_OUT_INIT_LOW
			     : GPIOF_OUT_INIT_HIGH,
			     "ctp_it7260_power")) {
		dev_err(dev, "no power pin available\n");
		ts->gpio.power->num = -EBUSY;
	}
}

 static int  Ctp_it7260_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct Ctp_it7260_data *Ctp_it7260;
	u8 ret;

	ret = i2c_check_functionality(client->adapter,
			I2C_FUNC_I2C);

	if (!ret) {
		dev_err(&client->dev, "SMBUS Byte Data not Supported\n");
		goto err_i2c;
	}

	Ctp_it7260_client = client;
	Ctp_it7260 = kzalloc(sizeof(struct Ctp_it7260_data), GFP_KERNEL);
	if (!Ctp_it7260) 
	{
		printk("[Ctp_it7260_probe]:alloc data failed.\n");
		goto err_alloc;
	}

	INIT_WORK(&Ctp_it7260->work, Ctp_it7260_work_func);
	Ctp_it7260->ctp_workq = create_singlethread_workqueue("ctp_it7260_workqueue");

	Ctp_it7260->client = client;
	i2c_set_clientdata(client, Ctp_it7260);

	ctp_it7260_pdata = client->dev.platform_data;
	if (ctp_it7260_pdata == NULL) {
		dev_err(&client->dev, "%s: platform data is null\n", __func__);
	}
/*
	if (!ctp_it7260_pdata->is_i2c_gpio)
		i2c_jz_setclk(client,300*1000);
*/

	Ctp_it7260_gpio_init(Ctp_it7260);

	if (!CaptouchHWInitial(client))
		goto  err_free_mem;

	ts_input_init(client);
 	ret = sysfs_create_group(&(client->dev).kobj, &Ctp_it7260_attr_group); 
	if(ret){
		dev_err(&client->dev, "failed to register sysfs\n");
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	Ctp_it7260->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	Ctp_it7260->early_suspend.suspend = Ctp_it7260_early_suspend;
	Ctp_it7260->early_suspend.resume= Ctp_it7260_late_resume;
	register_early_suspend(&Ctp_it7260->early_suspend);
#endif
       //enable_irq(client->irq);

 	//driver_create_file(&Ctp_it7260_driver.driver, &Ctp_it7260_attr_group);
 	ret = driver_create_file(&Ctp_it7260_driver.driver, (struct driver_attribute *)&dev_attr_upgrade);
 	ret = driver_create_file(&Ctp_it7260_driver.driver, (struct driver_attribute *)&dev_attr_calibration);

	if(!Ctp_it7260_init_irq(client)){
		goto err_irq_request;
	}

	printk("--- ctp 7260 ---probe success! \n");
	return 0;


err_free_mem:
	free_irq(client->irq,Ctp_it7260);
err_irq_request:
	kfree(Ctp_it7260);
err_alloc:

err_i2c:
	
	return false;

}

static int __init Ctp_it7260_init(void)
{
	int ret;	
	ret=i2c_add_driver(&Ctp_it7260_driver);
	return ret;
}

static void __exit Ctp_it7260_exit(void)
{
	/* We move these codes here because we want to detect the
	 * pen down event even when touch driver is not opened.
	 */
	i2c_del_driver(&Ctp_it7260_driver);
}

module_init(Ctp_it7260_init);
module_exit(Ctp_it7260_exit);

MODULE_AUTHOR("Robert_mu<robert.mu@rahotech.com>");

