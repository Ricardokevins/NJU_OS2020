#include "x86.h"
#include "fs.h"
#include "device.h"

#define SYS_WRITE 0
#define SYS_FORK 1
#define SYS_EXEC 2
#define SYS_SLEEP 3
#define SYS_EXIT 4
#define SYS_READ 5
#define SYS_SEM 6
#define SYS_GETPID 7
#define SYS_OPEN 8
#define SYS_LSEEK 9
#define SYS_CLOSE 10
#define SYS_REMOVE 11
#define SYS_LS 12

#define STD_OUT 0
#define STD_IN 1
#define SH_MEM 3

#define SEM_INIT 0
#define SEM_WAIT 1
#define SEM_POST 2
#define SEM_DESTROY 3

//some definition in File System
  
#define O_WRITE 0x01
#define O_READ 0x02
#define O_CREATE 0x04
#define O_DIRECTORY 0x08

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern Semaphore sem[MAX_SEM_NUM];
extern Device dev[MAX_DEV_NUM];

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

extern File file[MAX_FILE_NUM];
extern SuperBlock sBlock;

uint8_t shMem[MAX_SHMEM_SIZE];

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallRead(struct TrapFrame *tf);
void syscallFork(struct TrapFrame *tf);
void syscallExec(struct TrapFrame *tf);
void syscallSleep(struct TrapFrame *tf);
void syscallExit(struct TrapFrame *tf);
void syscallSem(struct TrapFrame *tf);
void syscallGetPid(struct TrapFrame *tf);

void syscallWriteStdOut(struct TrapFrame *tf);
void syscallReadStdIn(struct TrapFrame *tf);
void syscallWriteShMem(struct TrapFrame *tf);
void syscallReadShMem(struct TrapFrame *tf);

void syscallReadFile(struct TrapFrame *tf);
void syscallWriteFile(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

void syscallSemInit(struct TrapFrame *tf);
void syscallSemWait(struct TrapFrame *tf);
void syscallSemPost(struct TrapFrame *tf);
void syscallSemDestroy(struct TrapFrame *tf);

void syscallOpen(struct TrapFrame *tf);
void syscallLseek(struct TrapFrame *tf);
void syscallClose(struct TrapFrame *tf);
void syscallRemove(struct TrapFrame *tf);
void syscallLS(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)tf;

	switch(tf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf); // return
			break;
		case 0x20:
			timerHandle(tf);         // return or iret
			break;
		case 0x21:
			keyboardHandle(tf);      // return
			break;
		case 0x80:
			syscallHandle(tf);       // return
			break;
		default:assert(0);
	}

	pcb[current].stackTop = tmpStackTop;
}

void syscallHandle(struct TrapFrame *tf) 
{
	switch(tf->eax) 
	{ // syscall number
		case SYS_WRITE:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case SYS_READ:
			syscallRead(tf);
			break; // for SYS_READ
		case SYS_FORK:
			syscallFork(tf);
			break; // for SYS_FORK
		case SYS_EXEC:
			syscallExec(tf);
			break; // for SYS_EXEC
		case SYS_SLEEP:
			syscallSleep(tf);
			break; // for SYS_SLEEP
		case SYS_EXIT:
			syscallExit(tf);
			break; // for SYS_EXIT
		case SYS_SEM:
			syscallSem(tf);
			break; // for SYS_SEM
		case SYS_GETPID:
			syscallGetPid(tf);
			break; // for SYS_GETPID
		case SYS_OPEN:
			syscallOpen(tf);
			break;
		case SYS_LSEEK:
			syscallLseek(tf);
			break; // for SYS_SEEK
		case SYS_CLOSE:
			syscallClose(tf);
			break; // for SYS_CLOSE
		case SYS_REMOVE:
			syscallRemove(tf);
			break;
		case SYS_LS:
			syscallLS(tf);
			break; // for SYS_LS
		default:break;
	}
}

void timerHandle(struct TrapFrame *tf) {
	uint32_t tmpStackTop;
	int i = (current + 1) % MAX_PCB_NUM;
	while (i != current) {
		if (pcb[i].state == STATE_BLOCKED) {
			pcb[i].sleepTime--;
			if (pcb[i].sleepTime == 0) {
				pcb[i].state = STATE_RUNNABLE;
			}
		}
		i = (i + 1) % MAX_PCB_NUM;
	}

	if (pcb[current].state == STATE_RUNNING &&
			pcb[current].timeCount != MAX_TIME_COUNT) {
		pcb[current].timeCount++;
		return;
	}
	else {
		if (pcb[current].state == STATE_RUNNING) {
			pcb[current].state = STATE_RUNNABLE;
			pcb[current].timeCount = 0;
		}
		i = (current + 1) % MAX_PCB_NUM;
		while (i != current) {
			if (i != 0 && pcb[i].state == STATE_RUNNABLE) {
				break;
			}
			i = (i + 1) % MAX_PCB_NUM;
		}
		if (pcb[i].state != STATE_RUNNABLE) {
			i = 0;
		}
		current = i;
		// putChar('0' + current);
		pcb[current].state = STATE_RUNNING;
		pcb[current].timeCount = 1;
		tmpStackTop = pcb[current].stackTop;
		pcb[current].stackTop = pcb[current].prevStackTop;
		tss.esp0 = (uint32_t)&(pcb[current].stackTop);
		asm volatile("movl %0, %%esp"::"m"(tmpStackTop)); // switch kernel stack
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	}
	return;
}

void keyboardHandle(struct TrapFrame *tf) {
	//putString("INto KEY\n");
	uint32_t keyCode = getKeyCode();
	
	
	
	//if (getChar(keyCode) == 0) 
	//{
	//	putString("empty code\n");
	//	return;
	//}
	
	//putChar(getChar(keyCode));
	//putString("normal code\n");
	//putInt(getChar(keyCode));
	keyBuffer[bufferTail] = keyCode;
	bufferTail++;
	bufferTail=bufferTail%MAX_KEYBUFFER_SIZE;
	if (dev[STD_IN].value < 0) 
	{ 
		//putString("K block \n");
		dev[STD_IN].value ++;
		ProcessTable *pt = NULL;
		pt = (ProcessTable*)((uint32_t)(dev[STD_IN].pcb.prev) - (uint32_t)&(((ProcessTable*)0)->blocked));
		pt->state = STATE_RUNNABLE;
		

		dev[STD_IN].pcb.prev = (dev[STD_IN].pcb.prev)->prev;
		(dev[STD_IN].pcb.prev)->next = &(dev[STD_IN].pcb);
	}
	return;
}

void syscallWrite(struct TrapFrame *tf) 
{
	switch(tf->ecx) { // file descriptor
		case STD_OUT:
			if (dev[STD_OUT].state == 1) {
				syscallWriteStdOut(tf);
			}
			break; // for STD_OUT
		case SH_MEM:
			if (dev[SH_MEM].state == 1) {
				syscallWriteShMem(tf);
			}
			break; // for SH_MEM
		default:break;
	}
	if (tf->ecx >= MAX_DEV_NUM && tf->ecx < MAX_DEV_NUM + MAX_FILE_NUM) 
	{
		//putString("test 2\n");
		//putInt(tf->ecx);
		if (file[tf->ecx - MAX_DEV_NUM].state == 1)
			syscallWriteFile(tf);
	}
}

void syscallWriteStdOut(struct TrapFrame *tf) {
	int sel = tf->ds; //TODO segment selector for user data, need further modification
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		if (character == '\n') {
			displayRow++;
			displayCol = 0;
			if (displayRow == 25){
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80){
				displayRow++;
				displayCol = 0;
				if (displayRow == 25){
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
	}
	
	updateCursor(displayRow, displayCol);
	//TODO take care of return value
}

void syscallWriteShMem(struct TrapFrame *tf) {
	// TODO in lab4
	int sel = tf->ds;
	uint32_t size=tf->ebx;
	uint32_t index=tf->esi;
	uint8_t *buffer= (uint8_t *)tf->edx;
	uint8_t temp=0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for(int i=index;i<size+index;i++)
	{
		asm volatile("movb %%es:(%1), %0":"=r"(temp):"r"(buffer+i-index));
		shMem[i]=temp;
	}
	tf->eax=size;
}

void syscallRead(struct TrapFrame *tf) 
{
	switch(tf->ecx) 
	{
		case STD_IN:
			if (dev[STD_IN].state == 1) {
				syscallReadStdIn(tf);
			}
			break;
		case SH_MEM:
			if (dev[SH_MEM].state == 1) {
				syscallReadShMem(tf);
			}
			break;
		default:
			break;
	}
	//Hit file Read
	if (tf->ecx >= MAX_DEV_NUM && tf->ecx < MAX_DEV_NUM + MAX_FILE_NUM) 
	{ 
		if (file[tf->ecx - MAX_DEV_NUM].state == 1)
			syscallReadFile(tf);
	}
}

void syscallReadStdIn(struct TrapFrame *tf) {
	//putString("INto IN\n");
	
	if (dev[STD_IN].value == 0) 
	{ 
		//putString("No block\n");
		
		dev[STD_IN].value --;

		pcb[current].blocked.next = dev[STD_IN].pcb.next;
		pcb[current].blocked.prev = &(dev[STD_IN].pcb);
		dev[STD_IN].pcb.next = &(pcb[current].blocked);
		(pcb[current].blocked.next)->prev = &(pcb[current].blocked);

		pcb[current].state = STATE_BLOCKED;
		//call Timer to rechoose
		asm volatile("int $0x20");

		int size = tf->ebx; 
		int sel = tf->ds;
		char *str = (char*)tf->edx;
		
		int i = 0;
		char character = 0;
		asm volatile("movw %0, %%es"::"m"(sel));
		int flag=0;
		if(bufferHead==bufferTail)
		{
			flag=1;
		}
		for(;i < size-1;) 
		{
			if(bufferHead!=bufferTail)
			{ 
				character=getChar(keyBuffer[bufferHead]);
				//putString("char");
				putChar(character);
				bufferHead=(bufferHead+1)%MAX_KEYBUFFER_SIZE;
				if(character != 0) 
				{
					asm volatile("movb %0, %%es:(%1)"::"r"(character),"r"(str+i));
					i++;
				}
			}
			else
				break;
		}
		//putInt(i);
		//putString("hire\n");
		if(flag==0)
		{
			asm volatile("movb $0x00, %%es:(%0)"::"r"(str+i));
		}	
		//putInt(i);
		pcb[current].regs.eax = i;
		return;
	}
	else 
	{
		if (dev[STD_IN].value < 0) 
		{ 
			//putString(" blocked\n");
			pcb[current].regs.eax = -1;
			return;
		}
	}
}

void syscallReadShMem(struct TrapFrame *tf) {
	// TODO in lab4
	int sel = tf->ds;
	uint8_t *buffer= (uint8_t *)tf->edx;
	uint32_t size=tf->ebx;
	uint32_t index=tf->esi;
	uint8_t temp=0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for(int i=index;i<size+index;i++)
	{
		temp=shMem[i];
		asm volatile("movb %0, %%es:(%1)"::"r"(temp),"r"(buffer+i-index));
		
	}
	tf->eax=size;
}

void syscallFork(struct TrapFrame *tf) {
	int i, j;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].state == STATE_DEAD) {
			break;
		}
	}
	if (i != MAX_PCB_NUM) {
		pcb[i].state = STATE_PREPARING;

		enableInterrupt();
		for (j = 0; j < 0x100000; j++) {
			*(uint8_t *)(j + (i + 1) * 0x100000) = *(uint8_t *)(j + (current + 1) * 0x100000);
		}
		disableInterrupt();

		pcb[i].stackTop = (uint32_t)&(pcb[i].stackTop) -
			((uint32_t)&(pcb[current].stackTop) - pcb[current].stackTop);
		pcb[i].prevStackTop = (uint32_t)&(pcb[i].stackTop) -
			((uint32_t)&(pcb[current].stackTop) - pcb[current].prevStackTop);
		pcb[i].state = STATE_RUNNABLE;
		pcb[i].timeCount = pcb[current].timeCount;
		pcb[i].sleepTime = pcb[current].sleepTime;
		pcb[i].pid = i;
		pcb[i].regs.ss = USEL(2 + i * 2);
		pcb[i].regs.esp = pcb[current].regs.esp;
		pcb[i].regs.eflags = pcb[current].regs.eflags;
		pcb[i].regs.cs = USEL(1 + i * 2);
		pcb[i].regs.eip = pcb[current].regs.eip;
		pcb[i].regs.eax = pcb[current].regs.eax;
		pcb[i].regs.ecx = pcb[current].regs.ecx;
		pcb[i].regs.edx = pcb[current].regs.edx;
		pcb[i].regs.ebx = pcb[current].regs.ebx;
		pcb[i].regs.xxx = pcb[current].regs.xxx;
		pcb[i].regs.ebp = pcb[current].regs.ebp;
		pcb[i].regs.esi = pcb[current].regs.esi;
		pcb[i].regs.edi = pcb[current].regs.edi;
		pcb[i].regs.ds = USEL(2 + i * 2);
		pcb[i].regs.es = pcb[current].regs.es;
		pcb[i].regs.fs = pcb[current].regs.fs;
		pcb[i].regs.gs = pcb[current].regs.gs;
		/*XXX set return value */
		pcb[i].regs.eax = 0;
		pcb[current].regs.eax = i;
	}
	else {
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallExec(struct TrapFrame *tf) {
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	char tmp[128];
	int i = 0;
	char character = 0;
	int ret = 0;
	uint32_t entry = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	while (character != 0) {
		tmp[i] = character;
		i++;
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	}
	tmp[i] = 0;

	ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	if (ret == -1) {
		tf->eax = -1;
		return;
	}
	tf->eip = entry;
	return;
}

void syscallSleep(struct TrapFrame *tf) {
	if (tf->ecx == 0) {
		return;
	}
	else {
		pcb[current].state = STATE_BLOCKED;
		pcb[current].sleepTime = tf->ecx;
		asm volatile("int $0x20");
		return;
	}
	return;
}

void syscallExit(struct TrapFrame *tf) {
	pcb[current].state = STATE_DEAD;
	asm volatile("int $0x20");
	return;
}

void syscallSem(struct TrapFrame *tf) {
	switch(tf->ecx) {
		case SEM_INIT:
			syscallSemInit(tf);
			break;
		case SEM_WAIT:
			syscallSemWait(tf);
			break;
		case SEM_POST:
			syscallSemPost(tf);
			break;
		case SEM_DESTROY:
			syscallSemDestroy(tf);
			break;
		default:break;
	}
}

void syscallSemInit(struct TrapFrame *tf) {
	// TODO in lab4
	
	int flag=0;
	int pos=0;
	for (int i = 0; i < MAX_SEM_NUM ; i++) //find one which is vaild
	{
		if (sem[i].state == 0) 
		{
			flag=1;
			pos=i;
			break;
		}
			
	}
	//putInt(pos);
	if (flag) 
	{
		sem[pos].state = 1;
		sem[pos].value = (int32_t)tf->edx;
		sem[pos].pcb.prev = &(sem[pos].pcb);
		sem[pos].pcb.next = &(sem[pos].pcb);
		
		pcb[current].regs.eax = pos;
	}
	else
	{
			pcb[current].regs.eax = -1;
	}
	
	return;
}

void syscallSemWait(struct TrapFrame *tf) {
	// TODO in lab4
	int index = tf->edx;
	if (sem[index].state == 1) 
	{
		pcb[current].regs.eax = 0;
		sem[index].value--;
		if (sem[index].value < 0) 
		{
			pcb[current].blocked.next = sem[index].pcb.next;
			pcb[current].blocked.prev = &(sem[index].pcb);
			sem[index].pcb.next = &(pcb[current].blocked);
			(pcb[current].blocked.next)->prev = &(pcb[current].blocked);
			pcb[current].state = STATE_BLOCKED;
			asm volatile("int $0x20");
		}
	}
	else
	{
		//putString("Hit here");
		pcb[current].regs.eax = -1;	

	}
	
	return;
}

void syscallSemPost(struct TrapFrame *tf) {
	// TODO in lab4
	int index = tf->edx;
	if (sem[index].state == 1) 
	{
		pcb[current].regs.eax = 0;
		sem[index].value++;
		if (sem[index].value <= 0) 
		{
			ProcessTable *pt = NULL;
			pt = (ProcessTable*)((uint32_t)(sem[index].pcb.prev) - (uint32_t)&(((ProcessTable*)0)->blocked));
			pt->sleepTime = 0;
			pt->state = STATE_RUNNABLE;
			
			sem[index].pcb.prev = (sem[index].pcb.prev)->prev;
			(sem[index].pcb.prev)->next = &(sem[index].pcb);
		}
	}
	else
	{
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallSemDestroy(struct TrapFrame *tf) {
	// TODO in lab4
	int index = tf->edx;
	if (sem[index].state == 1) 
	{
		sem[index].state = 0;
		pcb[current].regs.eax = 0;	
		asm volatile("int $0x20");
	}
	else
	{//hit error
		pcb[current].regs.eax = -1;
	}
}

void syscallGetPid(struct TrapFrame *tf) {
	pcb[current].regs.eax = current;
	
	return;
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}


void syscallOpen(struct TrapFrame  *tf) 
{
	//Todo in lab5
	int ret = 0;
	int size = 0;
	int baseAddr = (current + 1) * 0x100000; 

	char *str = (char*)tf->ecx + baseAddr; 
	Inode fatherInode;
	Inode destInode;
	int fatherInodeOffset = 0;
	int destInodeOffset = 0;
	//String("path: ");
	//putString(str);
	//putChar('\n');
	ret = readInode(&sBlock, &destInode, &destInodeOffset, str);
	//putString("test in open inode offset\n");
	//putInt(destInodeOffset);
	//putInt(ret);
	//putString("end test\n");
	if(ret==-1)
	{
		putString("file not exsit\n");
	}
	else
	{
		putString("file already in\n");
	}
	
	int mode = tf->edx;
	if(ret==-1&&(!(mode & O_CREATE)))
	{
		tf->eax = -1;
		return;
	}
	if (ret == -1)
	{
		if (mode & O_CREATE) // create a new file
		{
			putString("Hit Create\n");
			if (mode & O_DIRECTORY)
			{
				if (str[stringLen(str) - 1] == '/')
					str[stringLen(str) - 1] = 0;
			}
			ret = stringChrR(str, '/', &size);
			if (size == stringLen(str))
			{
				tf->eax = -1;
				return;
			}
			char fatherPath[NAME_LENGTH];
			if (size == 0)
			{
				fatherPath[0] = '/';
				fatherPath[1] = 0;
			}
			else
				stringCpy(str, fatherPath, size);

			ret = readInode(&sBlock, &fatherInode, &fatherInodeOffset, fatherPath);
			if (ret < 0)
			{
				tf->eax = -1;
				return;
			}
			int temptype = 0;

			if (mode & O_DIRECTORY)
			{
				temptype = DIRECTORY_TYPE;
			}		
			else
			{
				temptype = REGULAR_TYPE;
			}	
			
			
			ret = allocInode(&sBlock,  &fatherInode, fatherInodeOffset, &destInode, &destInodeOffset, str + size + 1, temptype);
			putString("File not exsit And Create new one\n");
		}
		else
		{
			putString("Hit Bad Trap\n");
			return;
		}
		
	}

	//putString("sdada");
	//putInt(destInodeOffset);
	if (destInode.type == FIFO_TYPE || destInode.type == SOCKET_TYPE || destInode.type == UNKNOWN_TYPE)
	{
		putInt(destInode.type);
		putString("Error Type\n");
		tf->eax = -1;
		return;
	}
	
	if (mode & O_DIRECTORY)
	{
		if (destInode.type != DIRECTORY_TYPE)
		{
			tf->eax = -1;
			return;
		}
	}
	int pos=0;
	for (; pos < MAX_FILE_NUM; pos++)
	{
		if (file[pos].state == 0)
			break;
	}
		

	if (pos == MAX_FILE_NUM)
	{
		tf->eax = -1;
		return;
	}
	
	file[pos].state = 1;
	file[pos].inodeOffset = destInodeOffset;
	file[pos].offset = 0;
	file[pos].flags = mode;
	tf->eax = pos + MAX_DEV_NUM;
	return;
}

void syscallClose(struct TrapFrame *tf) 
{
	//Todo in lab5
	int fd=tf->ecx-MAX_DEV_NUM;
	if (fd >= MAX_FILE_NUM || fd < 0)
	{
		tf->eax = -1;
		return;
	}
	if(file[fd].state==0)
	{
		putString("Already closed!\n");
		tf->eax = -1;
		return;
	}
	file[fd].state = 0;
	tf->eax = 0;
	return ;
}

void syscallRemove(struct TrapFrame  *tf) 
{
	//Todo in lab5
	int ret = 0;
	int size = 0;
	int baseAddr = (current + 1) * 0x100000; 
	char *str = (char*)tf->ecx + baseAddr; 
	
	int fatherInodeOffset = 0;
	int destInodeOffset = 0;
	Inode fatherInode;
	Inode destInode;

	ret = readInode(&sBlock, &destInode, &destInodeOffset, str);
    if (ret < 0)
	{
		tf->eax = -1;
		return;
	}
	
	if (str[stringLen(str) - 1] == '/')
		str[stringLen(str) - 1] = 0;

	ret = stringChrR(str, '/', &size);
	if (size == stringLen(str))
	{
		tf->eax = -1;
		return;
	}

	char fatherPath[NAME_LENGTH];
	if (size == 0)
	{
		fatherPath[0] = '/';
		fatherPath[1] = 0;
	}
	else
	{
		stringCpy(str, fatherPath, size);
	}
	putString(fatherPath);
	ret = readInode(&sBlock, &fatherInode, &fatherInodeOffset, fatherPath);
	if (ret < 0)
	{
		tf->eax = -1;
		return;
	}
    
	int type = destInode.type;
	int rett=freeInode(&sBlock, &fatherInode, fatherInodeOffset, &destInode, &destInodeOffset, str + size + 1, type);
    putString("RM  !!!!!!!!!!!!!!!!!!\n");
	putInt(rett);
	if(rett==-1)
	{
		tf->eax = -1;
	}
	else
	{
		tf->eax = 0;
	}
	

	
	return ;
}

void syscallLseek(struct TrapFrame  *tf) 
{
	//Todo in lab5
	int fd = tf->ecx - MAX_DEV_NUM;
	if (file[fd].state == 0)
	{
		putString("already close And Failed to Lseek\n");
		tf->eax = -1;
		return;
	}

	int offset = tf->edx;
	int whence = tf->ebx;

	Inode inode;
	diskRead(&inode, sizeof(Inode), 1, file[fd].inodeOffset);

	if (whence == SEEK_CUR)
	{
		file[fd].offset += offset;
		tf->eax = file[fd].offset;
		return ;
	}
	if (whence == SEEK_SET)
	{
		file[fd].offset = offset;
		tf->eax = file[fd].offset;
		return ;
	}
	if (whence == SEEK_END && offset <= 0)
	{
		file[fd].offset = inode.size + offset;
		tf->eax = file[fd].offset;
		return ;
	}

	tf->eax = -1;
	return ;
}

void syscallWriteFile(struct TrapFrame *tf)
{
	//putString("temp\n");
	//putInt(tf->ecx);
	int pos=tf->ecx-MAX_DEV_NUM;
	//Judge Not authorize to Write or not
	if (file[pos].flags % 2 == 0) 
	{ 
		pcb[current].regs.eax = -1;
		return;
	}
	int baseAddr = (current + 1) * 0x100000; 
	uint8_t *str = (uint8_t*)tf->edx + baseAddr; 
	//putInt(baseAddr);
	int size = tf->ebx;
	//putString("test\n");
	//putString((char*)str);
	//putInt(size);
	uint8_t buffer[SECTOR_SIZE * SECTORS_PER_BLOCK];
	int block_index = file[pos].offset / sBlock.blockSize;
	int remainder = file[pos].offset % sBlock.blockSize;
	Inode inode;
	//putString("test ! ");
	//putInt(file[pos].inodeOffset);
	//diskRead((void*)&childInode, sizeof(Inode),1,sBlock.inodeTable * SECTOR_SIZE + (dir.inode - 1) * sizeof(Inode));
		
	diskRead((void*)&inode, sizeof(Inode), 1, file[pos].inodeOffset);
	//putInt(inode.blockCount);
	//putInt(inode.type);
	if (size <= 0) 
	{
		pcb[current].regs.eax = -1;
		return;
	}
	int temp_offset=file[pos].offset;
	//putString("pos1\n");
	//putInt();
	readBlock(&sBlock, &inode,block_index, buffer);
	int start = remainder;
	int cur = block_index;
	int left_num = size;
	int j = start;
	int ret=0;
	int i=0;
	while(left_num)
	{
		//putInt(inode.blockCount);
		if(cur>=inode.blockCount)
		{
			putString("Re Alloc!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			ret = allocBlock(&sBlock,&inode, file[pos].inodeOffset);
			if(ret<0)
			{
				putString("Test in Write Alloc Failed\n");
				inode.size = temp_offset + size - left_num > inode.size ? temp_offset + size - left_num : inode.size;
				tf->eax = size - left_num; // bytes have written
				diskWrite(&inode, sizeof(Inode), 1, file[pos].inodeOffset);
				return;	
			}
		}
		ret=readBlock(&sBlock,&inode,cur,buffer);
		//putString("test buffer content\n");
		//putString((char*)buffer);
		//putString("\n");
		//putString("pos2\n");
		//putInt(ret);

		int wbytes = sBlock.blockSize - start;
		if (wbytes >= left_num)
		{
			for (int k = 0; k < left_num; k++)
			{
				buffer[j] = str[i++];
				j++;
			}
			//putString("train buffer content\n");
			//putString((char*)buffer);
			//putString("\n");
			writeBlock(&sBlock, &inode, cur, buffer);
			inode.size = (temp_offset + size > inode.size) ? (temp_offset + size) : inode.size;
			//putInt(100000000);
			//putInt(inode.size);
			file[pos].offset += left_num;
			left_num = 0;
			tf->eax = size;
			diskWrite(&inode, sizeof(Inode), 1, file[pos].inodeOffset);
			return;
		}
		else
		{
			//Not enough free Space in cur block
			putString("Not enough in cur!!!!!\n");
			for (int k = 0; k < left_num; k++)
			{
				buffer[j] = str[i++];
				j++;
			}
			writeBlock(&sBlock, &inode, cur, buffer);
			file[pos].offset += wbytes;
			left_num -= wbytes;
			cur++; 
			// block index ++
			start = 0; 
			// start from a new block
			j = start;
			readBlock(&sBlock, &inode, cur, buffer);
		}
	}
	//putString("Hit unk Bad Trap in Test Write\n");
	tf->eax=-1;
	return;
}

void syscallReadFile(struct TrapFrame *tf)
{
	int pos=tf->ecx - MAX_DEV_NUM;
	//Judge Not authorize to read or not
	if ((file[pos].flags >> 1) % 2 == 0) 
	{ 
		pcb[current].regs.eax = -1;
		return;
	}

	
	int baseAddr = (current + 1) * 0x100000; 
	uint8_t *str = (uint8_t*)tf->edx + baseAddr; 
	int size = tf->ebx; 
	uint8_t buffer[SECTOR_SIZE * SECTORS_PER_BLOCK];

	int block_num = file[pos].offset / sBlock.blockSize;
	int remaind_num = file[pos].offset % sBlock.blockSize;
	
	Inode inode;
	diskRead(&inode, sizeof(Inode), 1, file[pos].inodeOffset);
	//putString("Read pos offset");
	//putInt(file[pos].inodeOffset);
	//putInt(inode.size);
	//offset can not larger than size
	if (inode.size <= file[pos].offset)
	{
		//putString("Hit bad Trap in Test Read\n");
		tf->eax = -1;
		return;
	}

	//putInt(block_num);
	readBlock(&sBlock, &inode, block_num, buffer);
	putString("Hello\n");
	putString((char*)buffer);
	int start = remaind_num; 
	int cur = block_num;
	
	int i=0;
	int left_num = size; 
	int block_pointer = start; 
	//putInt(block_pointer);
	while (left_num!=0)
	{
		int free_num = sBlock.blockSize - start;
		//All file data in one block
		if (free_num + file[pos].offset >= inode.size)
		{
			putString("Hit here\n");
			putInt(inode.size - file[pos].offset);
			putInt(block_pointer);
			for (int k = 0; k < inode.size - file[pos].offset; k++)
			{
				str[i] = buffer[block_pointer];
				block_pointer++;
				i++;
			}
			putString((char*)str);
			file[pos].offset = inode.size;//Read to the end
			tf->eax = size - left_num + inode.size - file[pos].offset;
			left_num = 0;
			return;
		}
		//all want to read in one block
		if (free_num >= left_num)
		{
			putString("Hit here2 \n");
			for (int k = 0; k < left_num; k++)
			{
				str[i] = buffer[block_pointer];
				block_pointer++;
				i++;
			}
			file[pos].offset += left_num;
			left_num = 0;
			tf->eax = size;
			return;
		}
		else
		{
			//Need more Loop time
			putString("Hit here3 \n");
			for (int k = 0; k < free_num; k++)
			{
				str[i] = buffer[block_pointer];
				block_pointer++;
				i++;
			}
			cur++;
			file[pos].offset += free_num;
			left_num -= free_num;	
			start = 0;
			block_pointer = start;
			readBlock(&sBlock, &inode, cur, buffer);
		}
	}
	//putString("Hit unk Bad Trap in Test Read\n");
	tf->eax = -1;
	return;
	
}

void syscallLS(struct TrapFrame *tf)
{
	//int i;
	int ret = 0;
	int baseAddr = (current + 1) * 0x100000; // base address of user process
	char *str = (char*)tf->ecx + baseAddr; // file path
	char *res = (char*)tf->edx + baseAddr;
	Inode inode;
	int inodeOffset = 0;

	ret = readInode(&sBlock, &inode, &inodeOffset, str);
	//putInt(inodeOffset);
	if (ret < 0)
	{
		tf->eax = -1;
		return;
	}

	if (inode.type != DIRECTORY_TYPE)
	{
		tf->eax = -1;
		return;
	}
	DirEntry dir;
	int dirIndex=0;
	char *p = res;
    while (getDirEntry(&sBlock, &inode, dirIndex, &dir) != -1) 
    {
        dirIndex ++;
		Inode childInode;
		diskRead((void*)&childInode, sizeof(Inode),1,sBlock.inodeTable * SECTOR_SIZE + (dir.inode - 1) * sizeof(Inode));
		stringCpy(dir.name, p, stringLen(dir.name));
		p += stringLen(dir.name);
		*p = ' ';
		p++;
		putString(dir.name);
		putInt(dir.inode);
		putInt(childInode.blockCount);
		putInt(sBlock.inodeTable * SECTOR_SIZE + (dir.inode - 1) * sizeof(Inode));

        //putString("Name: %s, Inode: %d, Type: %d, LinkCount: %d, BlockCount: %d, Size: %d.\n",dir.name, dir.inode, childInode.type, childInode.linkCount, childInode.blockCount, childInode.size);
    }
	
	
	//putString("doing LS!!!!!!!!!!!!!!!!!!\n");
	*p = 0;
	tf->eax = 0;
	return;
}