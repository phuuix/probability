/* samv.c
* Author: Xiong Puhui
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp.h"
#include "libsamv.h"

int samv_init_rxbuf(samv_rxbuf_t *rxbuf, u16_t size)
{
	rxbuf->mRxBuf = malloc(size);
	rxbuf->mRxBufSize = size;
	if(rxbuf->mRxBuf)
	{
		samv_reset_rxbuf(rxbuf);
		return 0;
	}

	return 1;
}

void samv_reset_rxbuf(samv_rxbuf_t *rxbuf)
{
	int i;
	
	rxbuf->mRxBytes = 0;
    rxbuf->mRxLength = 0;
    rxbuf->mRxState = SAMV_RXSTATE_NONE;
    for(i=0; i<10; i++)
    	rxbuf->mRxBuf[i] = 0;
}

u8_t samv_is_preamble(u8_t * buf)
{
	if(buf[0] == (u8_t)0xAA && 
			buf[1] == (u8_t)0xAA && 
			buf[2] == (u8_t)0xAA && 
			buf[3] == (u8_t)0x96 && 
			buf[4] == (u8_t)0x69)
		return 1;
	else
		return 0;
}

int samv_build_preamble(u8_t * msg)
{
	int bytes = 0;
	msg[bytes++] = (u8_t)0xAA;
	msg[bytes++] = (u8_t)0xAA;
	msg[bytes++] = (u8_t)0xAA;
	msg[bytes++] = (u8_t)0x96;
	msg[bytes++] = (u8_t)0x69;
	
	return bytes;
}

u8_t samv_calc_chksum(u8_t * msg, int offset, int bytes)
{
	int i;
	u8_t chksum = 0;
	
	for(i=offset; i<bytes; i++)
	{
		chksum = (u8_t)(chksum ^ msg[i]);
	}
	
	return chksum;
}

int samv_build_cmd_reset(u8_t * msg)
{
	int bytes;
	
	bytes = samv_build_preamble(msg);
	msg[bytes++] = 0;			//len1
	msg[bytes++] = 3;			//len2;
	msg[bytes++] = (u8_t)SAMV_CMD_RESET;		//cmd
	msg[bytes++] = (u8_t)SAMV_PARA_NONE;		//para
	msg[bytes] = (u8_t)samv_calc_chksum(msg, 5, bytes);
	
	return bytes+1;
}

int samv_build_cmd_status_detect(u8_t * msg)
{
	int bytes;
	
	bytes = samv_build_preamble(msg);
	msg[bytes++] = 0;			//len1
	msg[bytes++] = 3;			//len2;
	msg[bytes++] = (u8_t)SAMV_CMD_STATUS_DETECT;	//cmd
	msg[bytes++] = (u8_t)SAMV_PARA_NONE;			//para
	msg[bytes] = (u8_t)samv_calc_chksum(msg, 5, bytes);
	
	return bytes+1;
}

int samv_build_cmd_read_manageinfo(u8_t * msg)
{
	int bytes;
	
	bytes = samv_build_preamble(msg);
	msg[bytes++] = 0;			//len1
	msg[bytes++] = 3;			//len2;
	msg[bytes++] = (u8_t)SAMV_CMD_READ_MANAGEINFO;	//cmd
	msg[bytes++] = (u8_t)SAMV_PARA_NONE;				//para
	msg[bytes] = (u8_t)samv_calc_chksum(msg, 5, bytes);
	
	return bytes+1;
}

int samv_build_cmd_find_card(u8_t * msg)
{
	int bytes;
	
	bytes = samv_build_preamble(msg);
	msg[bytes++] = 0;			//len1
	msg[bytes++] = 3;			//len2;
	msg[bytes++] = (u8_t)SAMV_CMD_FIND_CARD;	//cmd
	msg[bytes++] = SAMV_PARA_FIND;			//para
	msg[bytes] = (u8_t)samv_calc_chksum(msg, 5, bytes);
	
	return bytes+1;
}

int samv_build_cmd_select_card(u8_t * msg)
{
	int bytes;
	
	bytes = samv_build_preamble(msg);
	msg[bytes++] = 0;			//len1
	msg[bytes++] = 3;			//len2;
	msg[bytes++] = (u8_t)SAMV_CMD_FIND_CARD;	//cmd
	msg[bytes++] = SAMV_PARA_SELECT;			//para
	msg[bytes] = (u8_t)samv_calc_chksum(msg, 5, bytes);
	
	return bytes+1;
}

int samv_build_cmd_read_info(u8_t * msg)
{
	int bytes;
	
	bytes = samv_build_preamble(msg);
	msg[bytes++] = 0;			//len1
	msg[bytes++] = 3;			//len2;
	msg[bytes++] = (u8_t)SAMV_CMD_READ_CARD;	//cmd
	msg[bytes++] = SAMV_PARA_INFO;			//para
	msg[bytes] = (u8_t)samv_calc_chksum(msg, 5, bytes);
	
	return bytes+1;
}

int samv_build_cmd_read_extinfo(u8_t * msg)
{
	int bytes;
	
	bytes = samv_build_preamble(msg);
	msg[bytes++] = 0;			//len1
	msg[bytes++] = 3;			//len2;
	msg[bytes++] = (u8_t)SAMV_CMD_READ_CARD;	//cmd
	msg[bytes++] = SAMV_PARA_EXTINFO;			//para
	msg[bytes] = (u8_t)samv_calc_chksum(msg, 5, bytes);
	
	return bytes+1;
}

int samv_build_cmd_read_card_mid(u8_t * msg)
{
	int bytes;
	
	bytes = samv_build_preamble(msg);
	msg[bytes++] = 0;			//len1
	msg[bytes++] = 3;			//len2;
	msg[bytes++] = (u8_t)SAMV_CMD_READ_CARD;	//cmd
	msg[bytes++] = SAMV_PARA_MID;			//para
	msg[bytes] = (u8_t)samv_calc_chksum(msg, 5, bytes);
	
	return bytes+1;
}

int samv_build_cmd_common(u8_t * msg, u8_t * data, u16_t length)
{
	u16_t bytes, i;
	
	// Set Preamble
	bytes = samv_build_preamble(msg);
	bytes += 2;				//length
	bytes += length;		//cmd+para+data
	
	// Set CMD, Parameter and Data
	for(i=0; i<length; i++)
	{
		msg[i+7] = data[i];
	}
	
	// Set Length
	msg[5] = (u8_t) ((length + 1) >> 8);
	msg[6] = (u8_t) ((length + 1) & 0xFF);
	
	// Set CHKSUM
	msg[bytes] = (u8_t)samv_calc_chksum(msg, 5, bytes);
	
	return bytes+1;
}

/* place received bytes into Rx buffer */
int samv_rx_bytes(samv_rxbuf_t *rxbuf, u8_t * rxmsg, int length)
{
	if(length + rxbuf->mRxBytes <= rxbuf->mRxBufSize)
	{
		memcpy(rxbuf->mRxBuf+rxbuf->mRxBytes, rxmsg, length);
		rxbuf->mRxBytes += length;
	}
	else
	{
		/* reset Rx buffer varaibles */
		samv_reset_rxbuf(rxbuf);
        return rxbuf->mRxState;
	}
	
	if(rxbuf->mRxState == SAMV_RXSTATE_NONE)
	{
		if(rxbuf->mRxBytes >= 5)
		{
			if(!samv_is_preamble(rxbuf->mRxBuf))
			{
				KPRINTF("samv: error: Invalid preamble\n");
				/* reset Rx buffer varaibles */
				samv_reset_rxbuf(rxbuf);
		        return rxbuf->mRxState;
			}
			
			rxbuf->mRxState = SAMV_RXSTATE_LENGTH;
		}
	}
	
	if(rxbuf->mRxState == SAMV_RXSTATE_LENGTH)
	{
		if(rxbuf->mRxBytes >= 7)
		{
			rxbuf->mRxLength = (rxbuf->mRxBuf[5] << 8) | (rxbuf->mRxBuf[6]) ;
			if(rxbuf->mRxLength <= 0 || rxbuf->mRxLength > rxbuf->mRxBufSize)
			{
				KPRINTF("samv: error: Invalid samv message length: %d\n", rxbuf->mRxLength);
				
				samv_reset_rxbuf(rxbuf);
		        return rxbuf->mRxState;
			}
			
			/* advance rx state */
			rxbuf->mRxState = SAMV_RXSTATE_BODY;
		}
	}
	
	if(rxbuf->mRxState == SAMV_RXSTATE_BODY)
	{
		if(rxbuf->mRxBytes >= rxbuf->mRxLength+7)
		{
			u8_t chksum;
			
			/* check sum */
			chksum = samv_calc_chksum(rxbuf->mRxBuf, 5, rxbuf->mRxBytes-1);
			if(chksum != rxbuf->mRxBuf[rxbuf->mRxBytes-1])
			{
				KPRINTF("samv: check sum error: chksum=%d\n", chksum);
				samv_reset_rxbuf(rxbuf);
		        return rxbuf->mRxState;
			}
			
			rxbuf->mRxState = SAMV_RXSTATE_OK;
		}
	}
	
	return rxbuf->mRxState;
}


int toHexString( char *from, int from_len, char *to, int to_len )
{
	static const char hexDigit[16] =
	{
		'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
	};
	u16_t i;
	
	if(from_len > (to_len>>1))
		from_len = (to_len>>1);
	
	for(i=0; i<from_len; i++)
	{
		to[i<<1] = hexDigit[from[i] >> 4];
		to[(i<<1)+1] = hexDigit[from[i] & 0xF];
	}
	
	return from_len;
}

int samv_build_answer(u8_t *msg, u8_t sw3, u8_t *data, u16_t length)
{
	int bytes;
	
	bytes = samv_build_preamble(msg);
	msg[bytes++] = (length+4)>>8;			//len1
	msg[bytes++] = (length+4)&0xFF;			//len2
	msg[bytes++] = 0;           //sw1
	msg[bytes++] = 0;           //sw2
	msg[bytes++] = sw3;         //sw3
	if(length>0 && data!=NULL)
		memcpy(&msg[bytes], data, length);
	bytes += length;
	msg[bytes] = (u8_t)samv_calc_chksum(msg, 5, bytes);
	
	return bytes+1;
}


