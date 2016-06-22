#include "cec.h"
//#define CEC_DEBUG

//register
static const u16 CEC_BASE = 0x7D00;
static const u8 CEC_CTRL = 0x00;
static const u8 CEC_MASK = 0x02;
static const u8 CEC_ADDR_L = 0x05;
static const u8 CEC_ADDR_H = 0x06;
static const u8 CEC_TX_CNT = 0x07;
static const u8 CEC_RX_CNT = 0x08;
static const u8 CEC_TX_DATA = 0x10;
static const u8 CEC_RX_DATA = 0x20;
static const u8 CEC_LOCK = 0x30;
static const u8 CEC_WKUPCTRL = 0x31;

static const u8 BCST_ADDR = 0x0F;
static const u8 DATA_SIZE = 16;
static const u8 STANDBY = 4;
static const u8 BC_NACK = 3;
static const u8 FRAME_TYP1 = (0x1 << 2);
static const u8 FRAME_TYP0 = (0x1 << 1);
static const u8 SEND = 0;
static const u8 CEC_POLARITY = 0x03;
static const u8 CEC_INT = 0x04;
static const u8 DONE = (0x1 << 0);
static const u8 EOM = (0x1 << 1);
static const u8 NACK = (0x1 << 2);
static const u8 ARB_LOST = (0x1 << 3);
static const u8 ERROR_FOLL = (0x1 << 5);
static const u8 ERROR_INIT = (0x1 << 4);
static const u8 WAKEUP = (0x1 << 6);
static const u8 LOCKED = 0;


void printk_cec(char *str)
{
	u8 i=0;
	printk("%s\n",str);
	printk("----CEC_CTRL    =%x\n",halCEC_ReadByte(CEC_CTRL));
	printk("----CEC_MASK    =%x\n",halCEC_ReadByte(CEC_MASK));
	printk("----CEC_ADDR_L  =%x\n",halCEC_ReadByte(CEC_ADDR_L));
	printk("----CEC_ADDR_H  =%x\n",halCEC_ReadByte(CEC_ADDR_H));

	printk("----CEC_TX_CNT  =%x\n",halCEC_ReadByte(CEC_TX_CNT));
	for(i=0;i<halCEC_ReadByte(CEC_TX_CNT);i++){
		printk("----CEC_TX_DATA[%d] =%x\n",i,halCEC_ReadByte(CEC_TX_DATA+i));
	}
#if 0
	if(send == 0){
		printk("----CEC_RX_CNT  =%x\n",halCEC_ReadByte(CEC_RX_CNT));
		for(i=0;i<halCEC_ReadByte(CEC_RX_CNT);i++){
			printk("----CEC_RX_DATA[%d] =%x\n",i,halCEC_ReadByte(CEC_RX_DATA+i));
		}
	}
#endif
	printk("----CEC_LOCK    =%x\n",halCEC_ReadByte(CEC_LOCK));
	printk("----CEC_WKUPCTRL=%x\n",halCEC_ReadByte(CEC_WKUPCTRL));
	printk("----Interrupt state	=%x\n\n",control_InterruptCecState(0));
}

void cec_init(void)
{
	u8 i = 0;
	halCEC_WriteByte(CEC_CTRL, 0x0);
	halCEC_WriteByte(CEC_MASK,WAKEUP);//disable wakeup
	control_InterruptCecClear(0, WAKEUP);
	/*halCEC_WriteByte(CEC_MASK,~(WAKEUP|DONE|EOM|NACK|ARB_LOST|ERROR_FOLL|ERROR_INIT));*/
	halCEC_WriteByte(CEC_MASK,~(WAKEUP|EOM|NACK|ARB_LOST|ERROR_FOLL|ERROR_INIT));
	halCEC_WriteByte(CEC_TX_CNT,0x00);
	halCEC_WriteByte(CEC_LOCK, 0x0);
	for(i=0;i<CEC_MAX_SIZE;i++){
		halCEC_WriteByte((CEC_TX_DATA+i),0x0);
	}
}

void sendmessage(u8 *value,int size)
{
	u8 i = 0,tx_size=size+1;
	if(size > CEC_MAX_SIZE){
		printk("error:cec max size is %d\n",CEC_MAX_SIZE);
		return;
	}
	cec_init();

	halCEC_WriteByte(CEC_CTRL, 0x12);

	switch(value[0]){
		case ACTIVE_SOURCE:
			printk("+cec send 'active source'+\n");
			halCEC_WriteByte(CEC_TX_DATA,0x4F);
			for(i=0;i<size;i++){
				halCEC_WriteByte(CEC_TX_DATA+i+1,value[i]);
			}
			halCEC_WriteByte(CEC_TX_CNT,tx_size);
			halCEC_WriteByte(CEC_ADDR_L,0x10);
			halCEC_WriteByte(CEC_ADDR_H,0x00);
			break;
		case IMAGE_VIEW_ON:
			printk("+cec send 'image view on(play)'+\n");
			halCEC_WriteByte(CEC_TX_DATA,0x40);
			for(i=0;i<size;i++){
				halCEC_WriteByte(CEC_TX_DATA+i+1,value[i]);
			}
			halCEC_WriteByte(CEC_TX_CNT,tx_size);
			halCEC_WriteByte(CEC_ADDR_L,0x10);
			halCEC_WriteByte(CEC_ADDR_H,0x00);
			break;
	    case CEC_STANDBY:
			printk("+cec send 'stand by'+\n");
			halCEC_WriteByte(CEC_TX_DATA,0x1F);
			for(i=0;i<size;i++){
				halCEC_WriteByte(CEC_TX_DATA+i+1,value[i]);
			}
			halCEC_WriteByte(CEC_TX_CNT,tx_size);
			halCEC_WriteByte(CEC_ADDR_L,0x02);
			halCEC_WriteByte(CEC_ADDR_H,0x00);
			break;
	    case INACTIVE_SOURCE:
			printk("+ cec send 'inactive source' +\n");
			halCEC_WriteByte(CEC_TX_DATA,0x40);
			for(i=0;i<size;i++){
				halCEC_WriteByte(CEC_TX_DATA+i+1,value[i]);
			}
			halCEC_WriteByte(CEC_TX_CNT,tx_size);
			halCEC_WriteByte(CEC_ADDR_L,0x10);
			halCEC_WriteByte(CEC_ADDR_H,0x00);
			break;

#if 0
		case :
			printk("+cec send 'get phy addr'+\n");
			halCEC_WriteByte(CEC_TX_DATA,0x40);
			halCEC_WriteByte(CEC_TX_DATA+1,0x83);
			halCEC_WriteByte(CEC_TX_CNT,0x02);
			halCEC_WriteByte(CEC_ADDR_L,0x10);
			halCEC_WriteByte(CEC_ADDR_H,0x0);
			break;
		case 1:
			printk("+++++polling++++\n");
			halCEC_WriteByte(CEC_TX_DATA,0x10);
			halCEC_WriteByte(CEC_TX_CNT,0x01);
			halCEC_WriteByte(CEC_ADDR_L,0x02);
			halCEC_WriteByte(CEC_ADDR_H,0x0);
			break;
		case 2:
			printk("+++++report phy++++\n");
			halCEC_WriteByte(CEC_TX_DATA,0x40);
			halCEC_WriteByte(CEC_TX_DATA+1,0x84);
			halCEC_WriteByte(CEC_TX_DATA+2,0x10);
			halCEC_WriteByte(CEC_TX_DATA+3,0x00);
			halCEC_WriteByte(CEC_TX_DATA+4,0x04);
			halCEC_WriteByte(CEC_TX_CNT,0x05);
			halCEC_WriteByte(CEC_ADDR_L,0x10);
			halCEC_WriteByte(CEC_ADDR_H,0x00);
			break;
#endif
		}

#ifdef CEC_DEBUG	
		printk_cec("-----prepare send");
#endif
		//send
		halCEC_WriteByte(CEC_CTRL,0x03);

		while(halCEC_ReadByte(CEC_CTRL) & (1<<SEND)){
			mdelay(100);
			if(i%10==0)
				printk("Waiting send message\n");
			i++;
			if(i>CEC_TIME_OUT){
				printk("Waiting send message timeout,your tv may not support cec !\n");
				break;
			}
		}
}

void cec_callback(void *param)
{
	int i = 0;

#ifdef CEC_DEBUG
	u8 state = *((u8*)(param));
	printk("----Call Back interrupt state =%x\n",state);
#endif

	while(halCEC_ReadByte(CEC_LOCK) != 1){
		mdelay(100);
		if(i%10==0)
			printk("Waiting CEC Receive Message\n");
		i++;
		if(i>CEC_TIME_OUT){
			break;
		}
	}

#ifdef CEC_DEBUG
	printk("----CEC_RX_CNT  =%x\n",halCEC_ReadByte(CEC_RX_CNT));
	for(i=0;i<halCEC_ReadByte(CEC_RX_CNT);i++){
		printk("----CEC_RX_DATA[%d] =%x\n",i,halCEC_ReadByte(CEC_RX_DATA+i));
	}
#endif

	if(halCEC_ReadByte(CEC_RX_CNT) !=0 && halCEC_ReadByte(CEC_RX_DATA+1) == 0){
			printk("!!!!!Feature Abort!!!!!!!!");
	}
	halCEC_WriteByte(CEC_LOCK, 0x0);

#if 0	
	receive = halCEC_ReadByte(CEC_RX_DATA+1);
	switch(receive){
		case  0x9F:
			sendmessage(9);
			break;
	}
#endif
}

void halCEC_WriteByte(u8 Addr, u8 bit)
{
	access_CoreWriteByte(bit, (Addr + CEC_BASE));
}
u8 halCEC_ReadByte(u8 Addr)
{
	return access_CoreReadByte(Addr + CEC_BASE);
}
