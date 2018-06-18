#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/fs.h>
#include <linux/interrupt.h>
#include "main.h"
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");


/* You may refer to Intel Sofware Manual for detailed description. */
struct idt_entry
{
	unsigned short lower16; /*lower 16 bits of interrupt handler */
	unsigned short sel;
	unsigned short flags:13;
	unsigned short dpl:2;
	unsigned short present:1;
	unsigned short higher16; /*higher 16 bits of interrupt handler */
} __attribute__((packed));

struct idt_desc
{
	unsigned short size;   /* size of idt table */
	unsigned long address; /* base address of idt table */
} __attribute__((packed));

struct ex_handler
{
	unsigned short sel;
	unsigned long pc;
} __attribute__((packed));


struct ex_handler orig_handler = {0, 0};
struct idt_desc *old_dtr;

/* write the base and size of idt table */
static void imp_load_idt(const struct idt_desc *dtr)
{
	asm volatile("lidt %0"::"m" (*dtr));
}

/* read the base and size of idt table  */
static void imp_store_idt(struct idt_desc *dtr)
{
	asm volatile("sidt %0":"=m" (*dtr));
}

static void unregister_handler(void)
{
	/* stop tracking divbyzero if enabled */
	if (orig_handler.pc != 0)
	{
     	printk("hello");		
		asm volatile("cli");
		struct idt_entry *entry = (struct idt_entry *)old_dtr->address;
		entry->dpl = 0;
		entry->sel = orig_handler.sel;
		unsigned long ohand = orig_handler.pc;
		entry->lower16 = ohand & 0x0000ffff;
		entry->higher16 = (ohand >> 16);
		imp_load_idt(old_dtr);
		orig_handler.pc = 0;
		orig_handler.sel = 0;
		asm volatile("sti");
	}
}

static void register_handler(struct ex_handler *h)
{
	/* start tracking divbyzero if not enabled */
	if (orig_handler.pc == 0)
	{
		old_dtr = vmalloc(sizeof(struct idt_desc *));
		asm volatile("cli");
		imp_store_idt(old_dtr);
		struct idt_entry *entry = (struct idt_entry *)old_dtr->address;
		orig_handler.sel = entry->sel;
		orig_handler.pc = (entry->higher16)<<16|(entry->lower16);
		//orig_handler.pc = h->pc;
		//orig_handler.sel = h->sel;
		entry->sel = h->sel;
		entry->dpl = 3;
 		//printk("%u\n",entry->sel);
     	//printk("%u\n",entry->dpl);
 		//printk("%u\n",orig_handler.sel);
		//printk("%lu\n",orig_handler.pc);
		unsigned long ohand = h->pc;
		entry->lower16 = ohand & 0x0000ffff;
		entry->higher16 = (ohand >> 16);
		//printk("%u\n",entry->lower16);
		//printk("%u\n",entry->higher16);
		imp_load_idt(old_dtr);
		asm volatile("sti");
	}
}

static long device_ioctl(struct file *file, unsigned int ioctl, unsigned long arg)
{
	void __user *argp = (void __user*)arg;
	struct ex_handler *au;
	struct ex_handler h = {0, 0};

	switch (ioctl)
	{
		case REGISTER_HANDLER:
     		au = (struct ex_handler *)(argp);
			h.sel = au->sel;
			h.pc = au->pc;
			/* TODO: copy argument from user */
			register_handler(&h);
			break;
		case UNREGISTER_HANDLER:
		    printk("hello");
         	au = NULL;
			unregister_handler();
			break;
		default:
			//printk("hello1");
			printk("NOT a valid ioctl\n");
			return 1;
	}
	return 0;
}

static struct file_operations fops = 
{
	.unlocked_ioctl = device_ioctl,
};

int start_module(void)
{
	if (register_chrdev(MAJOR_NUM, MODULE_NAME, &fops) < 0)
	{
		printk("PANIC: aos module loading failed\n");
		return 1;
	}
	printk("aos module loaded successfully\n");
	return 0;
}

void exit_module(void)
{
	unregister_handler();
	unregister_chrdev(MAJOR_NUM, MODULE_NAME);
	printk("aos module unloaded successfully\n");
}

module_init(start_module);
module_exit(exit_module);
