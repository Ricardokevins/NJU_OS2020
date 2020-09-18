#include "common.h"
#include "x86.h"
#include "device.h"
#include "fs.h"

void kEntry(void) {

	// Interruption is disabled in bootloader

	initSerial();// initialize serial port
	initIdt(); // initialize idt
	initIntr(); // iniialize 8259a
	initSeg(); // initialize gdt, tss
	initVga(); // initialize vga device
	initTimer();
	initKeyTable();
	initFS();
	loadUMain(); // load user program, enter user space

	while(1);
	assert(0);
}
