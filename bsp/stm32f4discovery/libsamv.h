/* libsamv.h
 */
 
#ifndef _LIBSAMV_H_
#define _LIBSAMV_H_

typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned long u32_t;

#define SAMV_RXBUF_SIZE 1600
#define SAMV_MAX_MSGSIZE 1500

enum{
	SW1POS = 7,
	SW2POS = 8,
	SW3POS = 9,
	
	/* commands */
	SAMV_CMD_RESET = 0x10,
	SAMV_CMD_STATUS_DETECT = 0x11,
	SAMV_CMD_READ_MANAGEINFO = 0x12,
	SAMV_CMD_FIND_CARD = 0x20,
	SAMV_CMD_READ_CARD = 0x30,
	
	/* parameters */
	SAMV_PARA_NONE = 0xFF,
	SAMV_PARA_FIND = 0x01,
	SAMV_PARA_SELECT = 0x02,
	SAMV_PARA_INFO = 0x01,
	SAMV_PARA_EXTINFO = 0x03,
	SAMV_PARA_MID = 0x05,
	
	/* rx state */
	SAMV_RXSTATE_NONE = 0,				//no intact preamble received
	SAMV_RXSTATE_LENGTH = 1,			//no intact length received
	SAMV_RXSTATE_BODY = 2,				//no intact body received
	SAMV_RXSTATE_OK = 3,				//one intact packet received
	SAMV_RXSTATE_ERROR = 4,             //error

	/* error code: sw3 */
	SAMV_ERRCODE_SUCCESS = 0x90,		   //操作成功
	SAMV_ERRCODE_CARD_SUCC = 0x9F,		   //寻找证/卡成功
	SAMV_ERRCODE_CHKSUM = 0x10,		   //接收业务终端数据的校验和错
	SAMV_ERRCODE_LENGTH = 0x11,	 	   //接收业务终端数据的长度错
	SAMV_ERRCODE_LOGICAL = 0x21,		   //接收业务终命令错误，包括命令中的各种数值或逻辑搭配错误
	SAMV_ERRCODE_PRIVILEDGE = 0x23,	   //越权操作
	SAMV_ERRCODE_UNKNOWN = 0x24,	 	   //无法识别的错误
	SAMV_ERRCODE_AUTHSAMV = 0x31,         //证/卡认证 SAM_V 失败
	SAMV_ERRCODE_AUTHCARD = 0x32,	 	   //SAM_V 认证 证/卡失败
	SAMV_ERRCODE_INFOVERIFY = 0x33,	   //信息验证错误
	SAMV_ERRCODE_CARDTYPE = 0x40,	 	   //无法识别的证卡类型
	SAMV_ERRCODE_READCARD = 0x41,		   //读证/卡操作失败
	SAMV_ERRCODE_RANDNUM = 0x47,		   //取随机数失败
	SAMV_ERRCODE_SELFTEST = 0x60,		   //SAM_V自检失败，不能接受命令
	SAMV_ERRCODE_UNAUTH = 0x66,		   //SAM_V没有经过授权，无法使用
	SAMV_ERRCODE_FINDCARD = 0x80,		   //寻找证/卡失败
	SAMV_ERRCODE_SELECTCARD = 0x81,	   //选取证/卡失败
	SAMV_ERRCODE_INVALIDITEM = 0x91,	   //证/卡中此项无内容
	SAMV_ERRCODE_COMMUNICATION = 0xFF,    //通信故障
};

typedef struct samv_rxbuf
{
	u8_t *mRxBuf;
	u16_t mRxBufSize;                       // the max size of rx buffer
	u16_t mRxBytes;							// the received bytes
    u16_t mRxLength;						// the Length in the header
    u16_t mRxState;
}samv_rxbuf_t;

int samv_init_rxbuf(samv_rxbuf_t *rxbuf, u16_t size);
void samv_reset_rxbuf(samv_rxbuf_t *rxbuf);
int samv_build_cmd_reset(u8_t * msg);
int samv_build_cmd_status_detect(u8_t * msg);
int samv_build_cmd_read_manageinfo(u8_t * msg);
int samv_build_cmd_find_card(u8_t * msg);
int samv_build_cmd_select_card(u8_t * msg);
int samv_build_cmd_read_info(u8_t * msg);
int samv_build_cmd_read_extinfo(u8_t * msg);
int samv_build_cmd_read_card_mid(u8_t * msg);
int samv_build_cmd_common(u8_t * msg, u8_t * data, u16_t length);
int samv_rx_bytes(samv_rxbuf_t *rxbuf, u8_t * rxmsg, int length);
int toHexString( char *from, int from_len, char *to, int to_len );
int samv_build_answer(u8_t *msg, u8_t sw3, u8_t *data, u16_t length);

#endif
