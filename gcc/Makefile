TARGET=h101
EXECUTABLE=$(TARGET).elf

OPENOCD_DIR=/usr/local/share/openocd
BUILD_DIR=build
$(shell mkdir -p $(BUILD_DIR) > /dev/null)

CC=arm-none-eabi-gcc
LD=arm-none-eabi-gcc
AR=arm-none-eabi-ar
AS=arm-none-eabi-as
CP=arm-none-eabi-objcopy
OD=arm-none-eabi-objdump


DEFS = -DUSE_STDPERIPH_DRIVER

MCU = cortex-m3
MCFLAGS = -mcpu=$(MCU) -mthumb -mlittle-endian -mthumb-interwork -ffast-math -fdata-sections -ffunction-sections

topdir = ..
INCLUDES = -I$(topdir)/H8mini-test/src/ \
	 -I$(topdir)/H8mini-acro/src/ \
	-I$(topdir)/CMSIS/ \
	-I$(topdir)/Peripherals/inc/

OPTIMIZE = -Os

DEPFLAGS = -MT $@ -MMD -MF $(BUILD_DIR)/$*.d
CFLAGS = $(MCFLAGS) $(DEPFLAGS) $(OPTIMIZE)  $(DEFS) $(INCLUDES) -Wl,-T,flash.ld,-Map,$(TARGET).map,--gc-sections
CXXFLAG = $(CFLAGS) -std=gnu99
AFLAGS = $(MCFLAGS)

SRC = 	$(wildcard $(topdir)/H101_dual/src/*.c) \
	$(wildcard $(topdir)/H101_dual/src/*.cpp) \
	$(topdir)/CMSIS/system_gd32f1x0.c \
	core_cm3.c \
	$(topdir)/Peripherals/src/gd32f1x0_adc.c \
	$(topdir)/Peripherals/src/gd32f1x0_dma.c \
	$(topdir)/Peripherals/src/gd32f1x0_fmc.c \
	$(topdir)/Peripherals/src/gd32f1x0_gpio.c \
	$(topdir)/Peripherals/src/gd32f1x0_i2c.c \
	$(topdir)/Peripherals/src/gd32f1x0_rcc.c \
	$(topdir)/Peripherals/src/gd32f1x0_timer.c \
	$(topdir)/Peripherals/src/gd32f1x0_usart.c \
	startup.s
VPATH = $(topdir)/H101_dual/src:$(topdir)/CMSIS:$(topdir)/Peripherals/src

OBJS = $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(notdir $(basename $(SRC)))))
DEPS = $(OBJS:.o=.d)

all: $(TARGET).hex

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ -lm -lnosys -o $@
$(TARGET): $(TARGET.elf)
	$(CP) -O binary $^ $@
$(TARGET).hex: $(EXECUTABLE)
	$(CP) -O ihex $^ $@
$(TARGET).flash: $(TARGET).hex
	openocd -f $(OPENOCD_DIR)/scripts/interface/stlink-v2.cfg -f $(OPENOCD_DIR)/scripts/target/stm32f1x.cfg -c init -c "reset halt" -c "flash write_image erase $(TARGET) 0x08000000" -c "verify_image $(TARGET) 0x08000000" -c "reset run" -c shutdown

unlock:
	openocd -s $(OPENOCD_DIR)/scripts -f $(OPENOCD_DIR)/scripts/interface/stlink-v2.cfg -f $(OPENOCD_DIR)/scripts/target/stm32f1x.cfg - debug_level 3 -c init -c "reset halt" -c "mww 0x40022004 0x45670123" -c "mww 0x40022004 0xCDEF89AB" -c "mww 0x40022008 0x45670123" -c "mww 0x40022008 0xCDEF89AB" -c "mww 0x40022010 0x220" -c "mww 0x40022010 0x260" -c "sleep 100" -c "mww 0x40022010 0x230" -c "mwh 0x1ffff800 0x5AA5" -c "sleep 1000" -c "mww 0x40022010 0x2220" -c "sleep 100" -c "mdw 0x40022010" -c "mdw 0x4002201c" -c "mdw 0x1ffff800" -c shutdown

clean:
	echo $(DEPS)
	rm -f Startup.lst $(TARGET) $(TARGET).lst $(TARGET).out $(TARGET).hex \
	      $(TARGET).map $(TARGET).dmp $(EXECUTABLE) $(OBJS) $(DEPS)
	rmdir $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	$(AS) $< -o $@

$(OBJS): Makefile
-include $(DEPS)
