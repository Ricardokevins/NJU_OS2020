#include "x86.h"
#include "device.h"

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallPrint(struct TrapFrame *tf);
void syscallFork(struct TrapFrame *tf);
void syscallExec(struct TrapFrame *tf);
void syscallSleep(struct TrapFrame *tf);
void syscallExit(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

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

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case 1:
			syscallFork(tf);
			break; // for SYS_FORK
		case 2:			
			syscallExec(tf);		
			break; // for SYS_EXEC
		case 3:
			syscallSleep(tf);
			break; // for SYS_SLEEP
		case 4:
			syscallExit(tf);
			break; // for SYS_EXIT
		default:break;
	}
}

void timerHandle(struct TrapFrame *tf) {
	// TODO in lab3
	for(int i=1;i<MAX_PCB_NUM;i++)
	{
		if(pcb[i].state==STATE_BLOCKED)
		{
			pcb[i].sleepTime-=1;
			if(pcb[i].sleepTime==0)
			{
				pcb[i].state=STATE_RUNNABLE;
			}
		}
	}
	pcb[current].timeCount+=1;
	if(pcb[current].timeCount>=MAX_TIME_COUNT||pcb[current].state==STATE_DEAD||pcb[current].state==STATE_BLOCKED||current==0)
	{	
		int pos=-1;			
		for(int i=1;i<MAX_PCB_NUM;i++)
		{		
			if(pcb[i].state==STATE_RUNNABLE && i!=current)
			{
				putString("Timer");
				putInt(i);
				if(pcb[current].state==STATE_RUNNING)
				{
					pcb[current].state=STATE_RUNNABLE;;
					pcb[current].timeCount=0;
				}			
				current=i;
				pos=i;
				pcb[i].state=STATE_RUNNING;
				uint32_t tmpStackTop = pcb[current].stackTop;
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
				return ;
			}
		}
		if(pos==-1)
		{
			if (current!=0)
				putString("No find\n");
			if(pcb[current].state==STATE_RUNNING)
			{
				pcb[current].state=STATE_RUNNABLE;;
				pcb[current].timeCount=0;
			}	
			current=0;
			uint32_t tmpStackTop = pcb[current].stackTop;
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
			return ;
		}
	}
	else
	{
		putString("NO change");
		putInt(current);
	}
	
	return;
}

void keyboardHandle(struct TrapFrame *tf) {
	uint32_t keyCode = getKeyCode();
	if (keyCode == 0)
		return;
	putChar(getChar(keyCode));
	keyBuffer[bufferTail] = keyCode;
	bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;
	return;
}

void syscallWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case 0:
			syscallPrint(tf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct TrapFrame *tf) {
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

void syscallFork(struct TrapFrame *tf) {
	int pos=-1;
	for(int i=1;i<MAX_PCB_NUM;i++)
	{
		if(pcb[i].state==STATE_DEAD)
		{
			pos=i;
			break;
		}
	}
	if(pos==-1)
	{
		pcb[current].regs.eax = -1;
		return;
	}
	else
	{
		putString("Fork");
		putInt(pos);
		enableInterrupt();
 		for (int j = 0; j < 0x100000; j++) 
		{
 			*(uint8_t *)(j + (pos + 1) * 0x100000) = *(uint8_t *)(j + (current + 1) *0x100000);
			if(j%0x10000==0)
 				asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
 		}
 		disableInterrupt();
		 /* 
		for (int j = 0; j < 0x100000; j++) 
		{
 			*(uint8_t *)(j + (pos + 1) * 0x100000) = *(uint8_t *)(j + (current + 1) *0x100000);
		}
		*/
		for(int i=0;i<MAX_STACK_SIZE;i++)
		{
			pcb[pos].stack[i] = pcb[current].stack[i];
		}
		pcb[pos].timeCount = 0;
		pcb[pos].sleepTime = 0;
		pcb[pos].pid = pos;
		pcb[pos].state = STATE_RUNNABLE;

		pcb[pos].stackTop=(uint32_t)&(pcb[pos].regs);
		pcb[pos].prevStackTop = (uint32_t)&(pcb[pos].stackTop);

		pcb[pos].regs.ss = USEL(2+2*pos);
		pcb[pos].regs.cs = USEL(2+2*pos);
		pcb[pos].regs.ds =USEL(2+2*pos);
		pcb[pos].regs.es =USEL(2+2*pos);
		pcb[pos].regs.fs =USEL(2+2*pos);
		pcb[pos].regs.gs =USEL(2+2*pos);
		pcb[pos].regs.eip = pcb[current].regs.eip;
		pcb[pos].regs.eflags = pcb[current].regs.eflags ;

		
		pcb[pos].regs.cs =pcb[current].regs.cs;
		/* 
		pcb[pos].regs.ds = pcb[current].regs.ds;
		pcb[pos].regs.es =pcb[current].regs.es;
		pcb[pos].regs.fs = pcb[current].regs.fs;
		pcb[pos].regs.gs = pcb[current].regs.gs;

		*/

		pcb[pos].regs.edi = pcb[current].regs.edi;
		pcb[pos].regs.esi = pcb[current].regs.esi;
		pcb[pos].regs.ebx = pcb[current].regs.ebx;
		pcb[pos].regs.edx = pcb[current].regs.edx;
		pcb[pos].regs.ecx = pcb[current].regs.ecx;
		pcb[pos].regs.esp = pcb[current].regs.esp;
		pcb[pos].regs.ebp = pcb[current].regs.ebp;
		//pcb[pos].regs.ss = pcb[current].regs.ss;
		

		//pcb[pos].regs.eip=pcb[current].regs.eip;
		//int dst_p=(pos + 1) * 0x100000;
		//int src_p=(current + 1) * 0x100000;
		
		 /* 
		for(int i=0;i<0x100000;i++)
		{
			*((uint8_t *)dst_p + i) = *((uint8_t *)src_p + i);
		}
		*/
		
		
		pcb[pos].regs.eax = 0;             // child process return value
    	pcb[current].regs.eax = pos;  // father process return value
		//putInt(pcb[pos].regs.eax);
		//pcb[pos].regs.eax = 0;             // child process return value
    	//pcb[current].regs.eax = pos;  // father process return value
		//pcb[current].state = STATE_RUNNABLE;
		//putInt(pcb[current].state);
		//putString("go timer");
		//asm volatile("int $0x20");
	}
	// TODO in lab3
	return;
}

void syscallExec(struct TrapFrame *tf) {
	int sel = tf->ds;
	asm volatile("movw %0, %%es"::"m"(sel));	
	// hint: ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	char *str = (char *)tf->edx;
	char dst[100];
	int i=0;
	char character='a';
	//asm volatile("movw %0, %%es"::"m"(sel));
	for(;character!='\0';i++)
	{
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));		
		dst[i]=character;
	}	
	uint32_t entry =0;
	int ret=loadElf(dst,(current + 1) * 0x100000, &entry);
	if (ret==0)
	{
		pcb[current].regs.eip=entry;
	}	
	else
	{
		tf->eax=-1;
	}
		
	return;
}

void syscallSleep(struct TrapFrame *tf) {
	// TODO in lab3
	putString("Sleep");
	putInt(current);
	pcb[current].sleepTime=tf->ecx;
	pcb[current].state=STATE_BLOCKED;
	asm volatile("int $0x20");
	return;
}

void syscallExit(struct TrapFrame *tf) {
	putString("Hit Exit\n");
	putInt(current);
	pcb[current].state=STATE_DEAD;
	asm volatile("int $0x20");
	// TODO in lab3
	return;
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}
