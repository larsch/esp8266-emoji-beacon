BOARD=d1_mini
UPLOAD_SPEED=460800
MONITOR_SPEED=115200
FLASH_DEF=4M3M
CFLAGS += -O3

monitor:
	picocom --baud $(MONITOR_SPEED) $(UPLOAD_PORT) --imap lfcrlf --echo

emojis.c emojis.h: generate.rb
	ruby generate.rb
