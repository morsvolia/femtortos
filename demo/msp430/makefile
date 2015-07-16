#TARGET = 001-two-tasks
TARGET = 002-two-tasks-preempt
OS_SRC = ../../src
CSRC = $(OS_SRC)/femtoRTOS.c $(OS_SRC)/femtoRTOSmsp430.c $(TARGET).c
CFG_INC = config-full
OS_INC = ../../include

MCU = msp430g2553

#cross toolchain path
CROSS_DIR = /opt/msp430-gcc-linux-3.3.4
CROSS = $(CROSS_DIR)/bin/msp430-elf-

CFLAGS = -c -Wall -O3 -g -I $(CFG_INC) -I $(OS_INC) -I $(CROSS_DIR)/include
ASFLAGS = 
LDFLAGS = -L$(CROSS_DIR)/include


#define program with prefix
CC = $(CROSS)gcc
AR = $(CROSS)ar
AS = $(CROSS)gcc -x assembler-with-cpp
SIZE = $(CROSS)size
RM = rm -f 

#add cpu info
CFLAGS += -mmcu=$(MCU)
ASFLAGS += -mmcu=$(MCU)
LDFLAGS += -mmcu=$(MCU)


OBJ = $(CSRC:.c=.o)

all: $(TARGET).elf	

$(TARGET).elf: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $(TARGET).elf
	@echo ELF file size-----------------------------
	@$(SIZE) $(TARGET).elf

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean all

clean:
	$(RM) $(TARGET).elf
	$(RM) $(TARGET).hex
	$(RM) *.o
	$(RM) *.d
	$(RM) $(OS_SRC)/*.o
	$(RM) $(OS_SRC)/*.d
