/****************************************************************************
 *  i386_stub.c
 *  gdb stub for i386, modified from sigops
 *
 *  To enable debugger support, two things need to happen.  One, a
 *  call to gdb_stub_init() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint().  Breakpoint()
 *  simulates a breakpoint by executing a trap #3.
 *
 *  Because gdb will sometimes write to the stack area to execute function
 *  calls, this program cannot rely on using the supervisor stack so it
 *  uses it's own stack area reserved in the int array remcomStack.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 *    Z             add break/watch-point
 *
 *    z             remove break point
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/
#define asm __asm__

#include <config.h>
#include <stdio.h>
#include <isr.h>
#include <list.h>
#include <bsp.h>

#include "i386_stub.h"
#include "i386_sel.h"

/************************************************************************
 * gdb stub interfece
 */
void (*gdb_get_packet)(char *);	/* get packet */
void (*gdb_put_packet)(char *);	/* put packet */

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 400

static char initialized;  /* boolean flag. != 0 means we've been initialized */

int     remote_debug = 0;
/*  debug >  0 prints ill-formed commands in valid packets & checksum errors */


static const char hexchars[]="0123456789abcdef";

/*
 * these should not be static cuz they can be used outside this module
 */
int gdb_registers[NUMREGBYTES/4];

char  remcomInBuffer[BUFMAX];
char  remcomOutBuffer[BUFMAX];

/* Address of a routine to RTE to if we get a memory fault.  */
static void (*volatile mem_fault_routine)() = NULL;

/* Indicate to caller of mem2hex or hex2mem that there has been an error.  */
static volatile int mem_err = 0;

/* valid zbreak list head and resource zbreak list head */
static struct zbreak valid_zbreak_list, resource_zbreak_list;
static struct zbreak zbreak_resource[MAX_ZBREAK_NUMBER];

static struct zbreak *zbreak_list_find(unsigned long address);
static struct zbreak *zbreak_new(unsigned long address);
static void zbreak_free(struct zbreak *z);

/*
static void waitabit();

static void waitabit()
{
  int i;
  for (i = 0; i < 1000; i++) ;
}
*/


char *gdb_strcpy(char *dst, const char *src)
{
	char *d = dst;
	
	while(*src)
		*dst++ = *src++;
		
	*dst = '\0';
		
	return d;
}


size_t gdb_strlen(const char *s)
{
	size_t n=0;
	
	while(*s)
	{
		n++;
		s++;
	}
	
	return n;
}


int hex(char ch)
{
  if ((ch >= 'a') && (ch <= 'f')) return (ch-'a'+10);
  if ((ch >= '0') && (ch <= '9')) return (ch-'0');
  if ((ch >= 'A') && (ch <= 'F')) return (ch-'A'+10);
  return (-1);
}


void set_mem_err ()
{
  mem_err = 1;
}

/* These are separate functions so that they are so short and sweet
   that the compiler won't save any registers (if there is a fault
   to mem_fault, they won't get restored, so there better not be any
   saved).  */
int get_char (char *addr)
{
  return *addr;
}

void
set_char (char *addr, int val)
{
  *addr = val;
}

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
/* If MAY_FAULT is non-zero, then we should set mem_err in response to
   a fault; if zero treat a fault like any other fault in the stub.  */
char* mem2hex(char *mem, char *buf, int count, int may_fault)
{
      int i;
      unsigned char ch;

      if (may_fault)
	  mem_fault_routine = set_mem_err;
      for (i=0;i<count;i++) {
          ch = get_char (mem++);
	  if (may_fault && mem_err)
	    return (buf);
          *buf++ = hexchars[ch >> 4];
          *buf++ = hexchars[ch % 16];
      }
      *buf = 0;
      if (may_fault)
	  mem_fault_routine = NULL;
      return(buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
char* hex2mem(char *buf, char *mem, int count, int may_fault)
{
      int i;
      unsigned char ch;

      if (may_fault)
	  mem_fault_routine = set_mem_err;
      for (i=0;i<count;i++) {
          ch = hex(*buf++) << 4;
          ch = ch + hex(*buf++);
          set_char (mem++, ch);
	  if (may_fault && mem_err)
	    return (mem);
      }
      if (may_fault)
	  mem_fault_routine = NULL;
      return(mem);
}

/* this function takes the 386 exception vector and attempts to
   translate this number into a unix compatible signal value */
int compute_signal(int exception_vector )
{
  int sigval;
  switch (exception_vector) {
    case 0 : sigval = 8; break; /* divide by zero */
    case 1 : sigval = 5; break; /* debug exception */
    case 3 : sigval = 5; break; /* breakpoint */
    case 4 : sigval = 16; break; /* into instruction (overflow) */
    case 5 : sigval = 16; break; /* bound instruction */
    case 6 : sigval = 4; break; /* Invalid opcode */
    case 7 : sigval = 8; break; /* coprocessor not available */
    case 8 : sigval = 7; break; /* double fault */
    case 9 : sigval = 11; break; /* coprocessor segment overrun */
    case 10 : sigval = 11; break; /* Invalid TSS */
    case 11 : sigval = 11; break; /* Segment not present */
    case 12 : sigval = 11; break; /* stack exception */
    case 13 : sigval = 11; break; /* general protection */
    case 14 : sigval = 11; break; /* page fault */
    case 16 : sigval = 7; break; /* coprocessor error */
    default:
      sigval = 7;         /* "software generated"*/
  }
  return (sigval);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
int hex2int(char **ptr, int *int_value)
{
    int num_chars = 0;
    int hex_value;

    *int_value = 0;

    while (**ptr)
    {
        hex_value = hex(**ptr);
        if (hex_value >=0)
        {
            *int_value = (*int_value <<4) | hex_value;
            num_chars ++;
        }
        else
            break;

        (*ptr)++;
    }

    return (num_chars);
}

/*
 * This function does all command procesing for interfacing to gdb.
 */
void gdb_stub_handler(int exception_vector)
{
  int    sigval;
  int    addr, length;
  char * ptr;

#ifdef GDB_REMOTE_UDP
  gdb_udp_poll_start();
#endif /* GDB_REMOTE_UDP */
  
  if(remote_debug) 
	  GDB_PRINTF("vector=%d, ps=0x%x, pc=0x%x\n", exception_vector, gdb_registers[PS], gdb_registers[PC]);

  /* reply to host that an exception has occurred */
  sigval = compute_signal(exception_vector);
  remcomOutBuffer[0] = 'S';
  remcomOutBuffer[1] =  hexchars[sigval >> 4];
  remcomOutBuffer[2] =  hexchars[sigval % 16];
  remcomOutBuffer[3] = 0;

  if(remote_debug)
	  GDB_PRINTF("GDB send packet: %s\n", remcomOutBuffer);
  gdb_put_packet(remcomOutBuffer);

  while (1) {
    remcomOutBuffer[0] = 0;
    gdb_get_packet(remcomInBuffer);
	if(remote_debug)
		GDB_PRINTF("GDB reveive packet: %s\n", remcomInBuffer);
    switch (remcomInBuffer[0]) {
      case '?' :   remcomOutBuffer[0] = 'S';
                   remcomOutBuffer[1] =  hexchars[sigval >> 4];
                   remcomOutBuffer[2] =  hexchars[sigval % 16];
                   remcomOutBuffer[3] = 0;
                 break;
      case 'd' : remote_debug = !(remote_debug);  /* toggle debug flag */
                 break;
      case 'g' : /* return the value of the CPU registers */
                mem2hex((char*) gdb_registers, remcomOutBuffer, NUMREGBYTES, 0);
                break;
      case 'G' : /* set the value of the CPU registers - return OK */
                hex2mem(&remcomInBuffer[1], (char*) gdb_registers, NUMREGBYTES, 0);
                gdb_strcpy(remcomOutBuffer,"OK");
                break;

      /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
      case 'm' :
		    /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
				ptr = &remcomInBuffer[1];
				if (hex2int(&ptr,&addr))
					if (*(ptr++) == ',')
						if (hex2int(&ptr,&length))
						{
							ptr = 0;
							mem_err = 0;
                            mem2hex((char*) addr, remcomOutBuffer, length, 1);

							if (mem_err) {
								gdb_strcpy (remcomOutBuffer, "E03");
								if(remote_debug)
									GDB_PRINTF("memory fault");
							}
                        }

				if (ptr)
				{
				  gdb_strcpy(remcomOutBuffer,"E01");
				  if(remote_debug)
					  GDB_PRINTF("malformed read memory command: %s",remcomInBuffer);
				}
	          break;

      /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
      case 'M' :
		    /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
			ptr = &remcomInBuffer[1];
			if (hex2int(&ptr,&addr))
				if (*(ptr++) == ',')
					if (hex2int(&ptr,&length))
						if (*(ptr++) == ':')
						{
							mem_err = 0;
							hex2mem(ptr, (char*) addr, length, 1);

							if (mem_err) {
							gdb_strcpy (remcomOutBuffer, "E03");
							if(remote_debug)
								GDB_PRINTF("memory fault");
							} else {
								gdb_strcpy(remcomOutBuffer,"OK");
							}

							ptr = 0;
						}
            if (ptr)
            {
		      gdb_strcpy(remcomOutBuffer,"E02");
		      if(remote_debug)
				  GDB_PRINTF("malformed write memory command: %s",remcomInBuffer);
		    }
            break;

      /* cAA..AA    Continue at address AA..AA(optional) */
      /* sAA..AA   Step one instruction from AA..AA(optional) */
      case 'c' :
      case 's' :
		/* try to read optional parameter, pc unchanged if no parm */
		ptr = &remcomInBuffer[1];
		if (hex2int(&ptr,&addr))
			gdb_registers[PC] = addr;


		/* set the trace bit if we're stepping */
		if (remcomInBuffer[0] == 's') 
			gdb_registers[PS] |= 0x100;
		else
			gdb_registers[PS] &= 0xfffffeff;

#ifdef GDB_REMOTE_UDP
  		gdb_udp_poll_stop();
#endif /* GDB_REMOTE_UDP */
		return;
//		break;

	  /* kill the program */
	  case 'k' :  /* do nothing */
		break;

	  /* Z<type-1x>,<address-x>,<length-x>    Add breakpoint */
	  case 'Z' :
		if(hex(remcomInBuffer[1]) != 0)
		{
			/* We support only software break points so far */
			break;
		}

		ptr = &remcomInBuffer[2];
		if (*(ptr++) == ',')
			if (hex2int(&ptr,&addr))
			{
				if (*(ptr++) == ',')
				{
					if (hex2int(&ptr,&length))
					{
						if(length != 1)
						{
							gdb_strcpy(remcomOutBuffer,"E02");
							if(remote_debug)
								GDB_PRINTF("Breakpoint address length isn't equal to 1.");
							break;
						}
						ptr = 0;
					}
				}
			}

		if (ptr)
		{
		  gdb_strcpy(remcomOutBuffer,"E01");
		  if(remote_debug)
			  GDB_PRINTF("malformed add breakpoint command: %s",remcomInBuffer);
		}
		else
		{
			struct zbreak *z;

			/* check whether this break point already set */
			z = zbreak_list_find(addr);
			if(z)
			{
				/* Repeated packet */
				gdb_strcpy(remcomOutBuffer,"OK");
				break;
			}
			
			/* get an free zbreak */
			mem_err = 0;
			z = zbreak_new(addr);
			if(z == NULL)
			{
				gdb_strcpy(remcomOutBuffer,"E03");
				if(remote_debug)
				  GDB_PRINTF("new zbreak failed.\n");
				break;
			}
			if (mem_err) {
				gdb_strcpy (remcomOutBuffer, "E03");
				if(remote_debug)
					GDB_PRINTF("memory fault");
				break;
			}
			gdb_strcpy(remcomOutBuffer, "OK");
		}
		break;

	  /* z<type-1x>,<address-x>,<length-x>    Remove breakpoint */
	  /* zz    Remove all breakpoints */
	  case 'z' : 
		if(hex(remcomInBuffer[1]) == 'z')
		{
			/* remove all breakpoints */
			while(valid_zbreak_list.next != &valid_zbreak_list)
			{
				zbreak_free(valid_zbreak_list.next);
			}
			gdb_strcpy(remcomOutBuffer, "OK");
			break;
		}

		if(hex(remcomInBuffer[1]) != 0)
		{
			/* We support only software break points so far */
			break;
		}

		ptr = &remcomInBuffer[2];
		if (*(ptr++) == ',')
			if (hex2int(&ptr,&addr))
			{
				if (*(ptr++) == ',')
				{
					if (hex2int(&ptr,&length))
					{
						if(length != 1)
						{
							gdb_strcpy(remcomOutBuffer,"E02");
							if(remote_debug)
								GDB_PRINTF("Breakpoint address length isn't equal to 1.");
							break;
						}
						ptr = 0;
					}
				}
			}

		if (ptr)
		{
		  gdb_strcpy(remcomOutBuffer,"E01");
		  if(remote_debug)
			  GDB_PRINTF("malformed remove breakpoint command: %s",remcomInBuffer);
		}
		else
		{
			struct zbreak *z;

			z = zbreak_list_find(addr);
			if(z == NULL)
			{
				gdb_strcpy(remcomOutBuffer,"E03");
				if(remote_debug)
				  GDB_PRINTF("Breakpoint no found.\n");
				break;
			}

			/* free zbreak */
			zbreak_free(z);

			gdb_strcpy(remcomOutBuffer,"OK");
		}
		break;
      } /* switch */

    /* reply to the request */
	if(remote_debug)
		GDB_PRINTF("GDB send packet: %s\n", remcomOutBuffer);
    gdb_put_packet(remcomOutBuffer);
    }
}

/* this function is used to set up exception handlers for tracing and
   breakpoints */
void gdb_stub_init()
{
	int i;

	gdb_registers[CS] = X_FLATCODE_SEL;
	gdb_registers[SS] = X_FLATDATA_SEL;
	gdb_registers[DS] = X_FLATDATA_SEL;
	gdb_registers[ES] = X_FLATDATA_SEL;
	gdb_registers[FS] = X_FLATDATA_SEL;
	gdb_registers[GS] = X_FLATDATA_SEL;

	bsp_esr_attach (1, gdb_stub_handler);
	bsp_esr_attach (3, gdb_stub_handler);

	/* init zbreak */
	DC_LINKLIST_INIT(&valid_zbreak_list, prev, next);
	SLINKLIST_INIT(&resource_zbreak_list, next);
	for(i=0; i<MAX_ZBREAK_NUMBER; i++)
	{
		SLINKLIST_INSERT(&resource_zbreak_list, next, &zbreak_resource[i]);
	}

#ifdef GDB_REMOTE_UDP
	gdb_init_udp();
#else /* GDB_REMOTE_UDP */
	gdb_init_serial();
#endif /* GDB_REMOTE_UDP */

	/* In case GDB is started before us, ack any packets (presumably
	 "$?#xx") sitting there.  */
//	gdb_put_packet ("+");

	initialized = 1;

}

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger. */
void breakpoint()
{
	if (initialized){
		BREAKPOINT();
	}
	else{
		uint32_t f;

		/* dump stack */

		f = bsp_fsave();
		while(1);
		bsp_frestore(f);		
	}

//	waitabit();
}

static struct zbreak *zbreak_list_find(unsigned long address)
{
	struct zbreak *z;

	/* check whether this break point already set */
	z = valid_zbreak_list.next;
	while(z != &valid_zbreak_list)
	{
		if(z->address == address)
		{
			return z;
		}
		z = z->next;
	}

	return NULL;
}

static struct zbreak *zbreak_new(unsigned long address)
{
	struct zbreak *z = NULL;

	/* get a zbreak */
	SLINKLIST_REMOVE(&resource_zbreak_list, next, &z);
	
	if(z)
	{
		z->address = address;

		/* save instruction to buffer */
		mem2hex((char *)address, (char *)z->buffer, 1, 0);

		/* set instruction to "int 3(cc)" */
		hex2mem("cc", (char*)address, 1, 1);
		
		DC_LINKLIST_INSERT(&valid_zbreak_list, prev, next, z);
	}
	return z;
}

static void zbreak_free(struct zbreak *z)
{
	if(z)
	{
		/* resore old instruction */
		hex2mem((char *)z->buffer, (char *)z->address, 1, 1);

		DC_LINKLIST_REMOVE(NULL, prev, next, z);

		SLINKLIST_INSERT(&resource_zbreak_list, next, z);
	}
}


