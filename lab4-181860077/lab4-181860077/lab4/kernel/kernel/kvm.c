#include "x86.h"
#include "device.h"
#include "fs.h"

SegDesc gdt[NR_SEGMENTS];       // the new GDT, NR_SEGMENTS=7, defined in x86/memory.h
TSS tss;

SuperBlock sBlock;

ProcessTable pcb[MAX_PCB_NUM]; // pcb
int current; // current process

Semaphore sem[MAX_SEM_NUM];
Device dev[MAX_DEV_NUM];

/*
MACRO
SEG(type, base, lim, dpl) (SegDesc) {...};
SEG_KCODE=1
SEG_KDATA=2
SEG_TSS=NR_SEGMENTS-1
DPL_KERN=0
DPL_USER=3
KSEL(desc) (((desc)<<3) | DPL_KERN)
USEL(desc) (((desc)<<3) | DPL_UERN)
asm [volatile] (AssemblerTemplate : OutputOperands [ : InputOperands [ : Clobbers ] ])
asm [volatile] (AssemblerTemplate : : InputOperands : Clobbers : GotoLabels)
*/
void initSeg() { // setup kernel segements
	int i = 0;
	gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);

	for (i = 1; i < MAX_PCB_NUM; i++) {
		gdt[1 + i * 2] = SEG(STA_X | STA_R, (i + 1) * 0x100000, 0x00100000, DPL_USER);
		gdt[2 + i * 2] = SEG(STA_W,         (i + 1) * 0x100000, 0x00100000, DPL_USER);
	}
	
	gdt[SEG_TSS] = SEG16(STS_T32A,      &tss, sizeof(TSS) - 1, DPL_KERN);
	gdt[SEG_TSS].s = 0;

	setGdt(gdt, sizeof(gdt)); // gdt is set in bootloader, here reset gdt in kernel

	/*
	 * 初始化TSS
	 */
	tss.ss0 = KSEL(SEG_KDATA);
	asm volatile("ltr %%ax":: "a" (KSEL(SEG_TSS)));

	/*设置正确的段寄存器*/
	asm volatile("movw %%ax,%%ds":: "a" (KSEL(SEG_KDATA)));
	asm volatile("movw %%ax,%%ss":: "a" (KSEL(SEG_KDATA)));

	lLdt(0);
}

void initFS () {
	readSuperBlock(&sBlock);
	// putInt(sBlock.sectorNum);
	// putInt(sBlock.inodeNum);
	// putInt(sBlock.blockNum);
	// putInt(sBlock.availInodeNum);
	// putInt(sBlock.availBlockNum);
	// putInt(sBlock.blockSize);
	// putInt(sBlock.inodeBitmap);
	// putInt(sBlock.blockBitmap);
	// putInt(sBlock.inodeTable);
	// putInt(sBlock.blocks);
}

void initSem() {
	int i;
	for (i = 0; i < MAX_SEM_NUM; i++) {
		sem[i].state = 0; // 0: not in use; 1: in use;
		sem[i].value = 0; // >=0: no process blocked; -1: 1 process blocked; -2: 2 process blocked;...
		sem[i].pcb.next = &(sem[i].pcb);
		sem[i].pcb.prev = &(sem[i].pcb);
	}
}

void initDev() {
	int i;
	for (i = 0; i < MAX_DEV_NUM; i++) {
		dev[i].state = 1; // 0: not in use; 1: in use;
		dev[i].value = 0; // >=0: no blocked; -1: 1 process blocked; -2: 2 process blocked;...
		dev[i].pcb.next = &(dev[i].pcb);
		dev[i].pcb.prev = &(dev[i].pcb);
	}
}

int loadElf(const char *filename, uint32_t physAddr, uint32_t *entry) {
	int i = 0;
	int j = 0;
	uint8_t buf[sBlock.blockSize];
	int ret = 0;
	Inode inode;
	int inodeOffset = 0;
	struct ELFHeader *eh = NULL;
	struct ProgramHeader *ph = NULL;
	putString("\nload ");
	putString(filename);
	putChar('\n');
	ret = readInode(&sBlock, &inode, &inodeOffset, filename);
	if (ret == -1) {
		return -1;
	}
	ret = readBlock(&sBlock, &inode, 0, buf);
	if (ret == -1) {
		return -1;
	}
	eh = (struct ELFHeader *)buf;
	if (eh->magic != 0x464c457f) {
		return -1;
	}

	for (i = 0; i < inode.blockCount; i++) {
		readBlock(&sBlock, &inode, i, (uint8_t *)(physAddr + i * sBlock.blockSize));
	}
	ph = (struct ProgramHeader *)(buf + eh->phoff);
	for (i = 0; i < eh->phnum; i++) {
		if (ph->type == 0x1 && ph->vaddr != ph->off) {
			for (j = 0; j < ph->filesz; j++) {
				*(uint8_t *)(physAddr + ph->vaddr + j) = *(uint8_t *)(physAddr + ph->off + j);
			}
			for (j = ph->filesz; j < ph->memsz; j++) {
				*(uint8_t *)(physAddr + ph->vaddr + j) = 0;
			}
		}
		ph++;
	}
	*entry = eh->entry;
	return 0;
}

/*
kernel is loaded to location 0x100000, i.e., 1MB
size of kernel is not greater than 200*512 bytes, i.e., 100KB
user program is loaded to location 0x200000, i.e., 2MB
size of user program is not greater than 200*512 bytes, i.e., 100KB
*/

uint32_t loadUMain(void) {
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

	// enterUserSpace(uMainEntry);
	return uMainEntry;
}

void initProc() {
	int i = 0;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		pcb[i].state = STATE_DEAD;
	}
	// kernel process
	pcb[0].stackTop = (uint32_t)&(pcb[0].stackTop);
	pcb[0].prevStackTop = (uint32_t)&(pcb[0].stackTop);
	pcb[0].state = STATE_RUNNING;
	pcb[0].timeCount = MAX_TIME_COUNT;
	pcb[0].sleepTime = 0;
	pcb[0].pid = 0;
	// user process
	pcb[1].stackTop = (uint32_t)&(pcb[1].regs);
	pcb[1].prevStackTop = (uint32_t)&(pcb[1].stackTop);
	pcb[1].state = STATE_RUNNABLE;
	pcb[1].timeCount = 0;
	pcb[1].sleepTime = 0;
	pcb[1].pid = 1;
	pcb[1].regs.ss = USEL(4);
	pcb[1].regs.esp = 0x100000;
	asm volatile("pushfl");
	asm volatile("popl %0":"=r"(pcb[1].regs.eflags));
	pcb[1].regs.eflags = pcb[1].regs.eflags | 0x200;
	pcb[1].regs.cs = USEL(3);
	pcb[1].regs.eip = loadUMain();
	pcb[1].regs.ds = USEL(4);
	pcb[1].regs.es = USEL(4);
	pcb[1].regs.fs = USEL(4);
	pcb[1].regs.gs = USEL(4);

	current = 0;
	asm volatile("movl %0, %%esp"::"m"(pcb[0].stackTop));
	enableInterrupt();
	asm volatile("int $0x20");
	while(1) {
		waitForInterrupt();
	}
}
