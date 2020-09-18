#include "x86.h"
#include "device.h"
#include "fs.h"

SegDesc gdt[NR_SEGMENTS];       // the new GDT, NR_SEGMENTS=7, defined in x86/memory.h
TSS tss;

SuperBlock sBlock;

/*
MACRO
SEG(type, base, lim, dpl) (SegDesc) {...};
SEG_KCODE=1
SEG_KDATA=2
SEG_UCODE=3
SEG_UDATA=4
SEG_TSS=5
DPL_KERN=0
DPL_USER=3
KSEL(desc) (((desc)<<3) | DPL_KERN)
USEL(desc) (((desc)<<3) | DPL_UERN)
asm [volatile] (AssemblerTemplate : OutputOperands [ : InputOperands [ : Clobbers ] ])
asm [volatile] (AssemblerTemplate : : InputOperands : Clobbers : GotoLabels)
*/
void initSeg() { // setup kernel segements
	gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);
	//gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_USER);
	gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0x00200000,0x000fffff, DPL_USER);
	//gdt[SEG_UDATA] = SEG(STA_W,         0,       0xffffffff, DPL_USER);
	gdt[SEG_UDATA] = SEG(STA_W,         0x00200000,0x000fffff, DPL_USER);
	gdt[SEG_TSS] = SEG16(STS_T32A,      &tss, sizeof(TSS)-1, DPL_KERN);
	gdt[SEG_TSS].s = 0;
	setGdt(gdt, sizeof(gdt)); // gdt is set in bootloader, here reset gdt in kernel

	/*
	 * 初始化TSS
	 */
	tss.esp0 = 0x1fffff; //TODO top of kernel stack, further modification
	tss.ss0 = KSEL(SEG_KDATA);
	asm volatile("ltr %%ax":: "a" (KSEL(SEG_TSS)));

	/*设置正确的段寄存器*/
	asm volatile("movw %%ax,%%ds":: "a" (KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax,%%es":: "a" (KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax,%%fs":: "a" (KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax,%%gs":: "a" (KSEL(SEG_KDATA)));
	asm volatile("movw %%ax,%%ss":: "a" (KSEL(SEG_KDATA)));

	lLdt(0);
	
}

void initFS () {
	readSuperBlock(&sBlock);
	putInt(sBlock.sectorNum);
	putInt(sBlock.inodeNum);
	putInt(sBlock.blockNum);
	putInt(sBlock.availInodeNum);
	putInt(sBlock.availBlockNum);
	putInt(sBlock.blockSize);
	putInt(sBlock.inodeBitmap);
	putInt(sBlock.blockBitmap);
	putInt(sBlock.inodeTable);
	putInt(sBlock.blocks);
}

void enterUserSpace(uint32_t entry) {
	/*
	 * Before enter user space 
	 * you should set the right segment registers here
	 * and use 'iret' to jump to ring3
	 */
	uint32_t EFLAGS = 0;
	asm volatile("pushl %0":: "r" (USEL(SEG_UDATA))); // push ss
	asm volatile("pushl %0":: "r" (0x200000)); //TODO push esp, further modification
	asm volatile("pushfl"); //push eflags, sti
	asm volatile("popl %0":"=r" (EFLAGS));
	asm volatile("pushl %0"::"r"(EFLAGS|0x200));
	asm volatile("pushl %0":: "r" (USEL(SEG_UCODE))); // push cs
	asm volatile("pushl %0":: "r" (entry)); //TODO push eip, further modification
	//asm volatile("pushl %0":: "r" (0x200000));
	//asm volatile("pushl %0":: "r" (0x000000));
	//asm volatile("sti"); //TODO may be not necessary
	asm volatile("pushl %0":: "r" (USEL(SEG_UDATA)));
	asm volatile("pushl %0":: "r" (USEL(SEG_UDATA)));
	asm volatile("pushl %0":: "r" (USEL(SEG_UDATA)));
	asm volatile("pushl %0":: "r" (USEL(SEG_UDATA)));
	asm volatile("popl %gs");
	asm volatile("popl %fs");
	asm volatile("popl %es");
	asm volatile("popl %ds");
	// asm volatile("sti"); //TODO may be not necessary
	asm volatile("iret");
}

/*
kernel is loaded to location 0x100000, i.e., 1MB
size of kernel is not greater than 200*512 bytes, i.e., 100KB
user program is loaded to location 0x200000, i.e., 2MB
size of user program is not greater than 200*512 bytes, i.e., 100KB
*/

void loadUMain(void) {
	int i = 0;
	int phoff = 0x34; // program header offset
	int offset = 0x1000; // .text section offset
	uint32_t elf = 0x200000; // physical memory addr to load
	uint32_t uMainEntry = 0x200000;
	Inode inode;
	int inodeOffset = 0;

	readInode(&sBlock, &inode, &inodeOffset, "/boot/initrd");

	for (i = 0; i < inode.blockCount; i++) {
		readBlock(&sBlock, &inode, i, (uint8_t *)(elf + i * sBlock.blockSize));
	}
	
	uMainEntry = ((struct ELFHeader *)elf)->entry; // entry address of the program
	phoff = ((struct ELFHeader *)elf)->phoff;
	offset = ((struct ProgramHeader *)(elf + phoff))->off;

	for (i = 0; i < 200 * 512; i++) {
		*(uint8_t *)(elf + i) = *(uint8_t *)(elf + i + offset);
	}

	enterUserSpace(uMainEntry);
}
