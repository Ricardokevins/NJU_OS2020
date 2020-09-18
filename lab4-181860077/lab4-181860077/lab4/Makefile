QEMU = qemu-system-i386

os.img:
	@cd utils/genFS; make
	@cd bootloader; make
	@cd kernel; make
	@cd bounded_buffer; make
	@cd philosopher; make
	@cd reader_writer; make
	@cd app_print; make
	@cd app; make
	@#cat bootloader/bootloader.bin kernel/kMain.bin app/uMain.bin > os.img
	@#cat bootloader/bootloader.bin kernel/kMain.bin app/uMain.elf > os.img
	@#cat bootloader/bootloader.bin kernel/kMain.elf app/uMain.elf > os.img
	@./utils/genFS/genFS app/uMain.elf app_print/app_print.elf bounded_buffer/bounded_buffer.elf philosopher/philosopher.elf reader_writer/reader_writer.elf
	cat bootloader/bootloader.bin kernel/kMain.elf fs.bin > os.img

play: os.img
	$(QEMU) -serial stdio os.img

debug: os.img
	$(QEMU) -serial stdio -s -S os.img

clean:
	@cd utils/genFS; make clean
	@cd bootloader; make clean
	@cd kernel; make clean
	@cd bounded_buffer; make clean
	@cd philosopher; make clean
	@cd reader_writer; make clean
	@cd app_print; make clean
	@cd app; make clean
	rm -f fs.bin os.img
