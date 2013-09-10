/*	PIC management code & data	*/

#include <irq.h>
#include <sys/types.h>
#include <i386_data.h>
#include <i386_hw.h>

#define ICW1_M 	0x020 /* Master PIC (8259) register settings */
#define ICW2_M  0x021
#define ICW3_M  0x021
#define ICW4_M  0x021
#define OCW1_M  0x021
#define OCW2_M  0x020
#define OCW3_M  0x020

#define ICW1_S  0x0A0 /* Slave PIC register setting */
#define ICW2_S  0x0A1
#define ICW3_S  0x0A1
#define ICW4_S  0x0A1
#define OCW1_S  0x0A1
#define OCW2_S  0x0A0
#define OCW3_S  0x0A0

#define PIC1_BASE 0x040 /* Interrupt base for each PIC in HARTIK 3.0 */
#define PIC2_BASE 0x070 
#define EOI       0x020 /* End Of Interrupt code for PIC! */

#define bit_on(v,b)     ((v) |= (1 << (b)))
#define bit_off(v,b)    ((v) &= ~(1 << (b)))

/* PIC interrupt mask */
BYTE PIC_master_mask = 0xFF;
BYTE PIC_slave_mask = 0xFF;


void PIC_init(void)
{
    outp(ICW1_M,0x11);
    outp(ICW2_M,PIC1_BASE);
    outp(ICW3_M,0x04);
    outp(ICW4_M,0x01);
    outp(OCW1_M,0xFF);

    outp(ICW1_S,0x11);
    outp(ICW2_S,PIC2_BASE);
    outp(ICW3_S,0x02);
    outp(ICW4_S,0x01);
    outp(OCW1_S,0xFF);
}

void PIC_end(void)
{
    outp(ICW1_M,0x11);
    outp(ICW2_M,0x08);
    outp(ICW3_M,0x04);
    outp(ICW4_M,0x01);
    outp(OCW1_M,0xFF);

    outp(ICW1_S,0x11);
    outp(ICW2_S,0x70);
    outp(ICW3_S,0x02);
    outp(ICW4_S,0x01);
    outp(OCW1_S,0xFF);
}

void bsp_irq_mask(WORD irqno)
{
    /* Interrupt is on master PIC */
    if (irqno < 8) {
		bit_on(PIC_master_mask,irqno);
		outp(0x21,PIC_master_mask);
    } else if (irqno < 16) {
		/* Interrupt on slave PIC */
		bit_on(PIC_slave_mask,irqno-8);
		outp(0xA1,PIC_slave_mask);
		/* If the slave PIC is completely off   */
		/* Then turn off cascading line (Irq #2)*/
		if (PIC_slave_mask == 0xFF && !(PIC_master_mask & 0x04)) {
			bit_on(PIC_master_mask,2);
			outp(0x21,PIC_master_mask);
		}
    }
}

void bsp_irq_unmask(WORD irqno)
{
    /* Interrupt is on master PIC */
    if (irqno < 8) {
		bit_off(PIC_master_mask,irqno);
		outp(0x21,PIC_master_mask);
    } else if (irqno < 16) {
		/* Interrupt on slave PIC */
		bit_off(PIC_slave_mask,irqno-8);
		outp(0xA1,PIC_slave_mask);
		/* If the cascading irq line was off */
		/* Then activate it also!            */
		if (PIC_master_mask & 0x04) {
			bit_off(PIC_master_mask,2);
			outp(0x21,PIC_master_mask);
		}
    }
}
