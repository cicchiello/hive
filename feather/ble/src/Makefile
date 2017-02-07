ROOT := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

SKETCH=hive
DFAMILY=samd21

LOCALOBJS=CpuTempSensor.o Timestamp.o Sensor.o Actuator.o \
	HumidSensor.o TempSensor.o \
	StepperMonitor.o StepperActuator.o \
	SensorRateActuator.o hive_platform.o \
	beecnt.o latch.o ServoConfigActuator.o \
	BleStream.o AudioActuator.o Trace.o sine.o
MYPACKAGES=strutils couchutils tests platformutils cloudpipe DHT
OTHERPACKAGES=SPI arduino arduino_zero USB Wire
PACKAGES=$(MYPACKAGES) $(OTHERPACKAGES)

LIBOBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp)) $(patsubst %.c,%.o,$(wildcard *.c))

HPACKAGES=
CPACKAGES=Adafruit_BluefruitLE_nRF51 Adafruit_Motorshield
PACKAGE_LIBS=$(addsuffix .lib,$(PACKAGES) $(HPACKAGES) $(CPACKAGES))

ARDUINO_PACKAGES=/home/pi/.arduino15/packages
ARDUINO_TOOLS=$(ARDUINO_PACKAGES)/arduino/tools
ADAFRUIT_TOOLS=$(ARDUINO_PACKAGES)/adafruit/hardware/samd/1.0.13

CPP=$(ARDUINO_TOOLS)/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-g++
CC=$(ARDUINO_TOOLS)/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-gcc
AR=$(ARDUINO_TOOLS)/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-ar
LD_SCRIPT=$(ROOT)/flash_with_bootloader.ld
OBJCPY=$(ARDUINO_TOOLS)/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-objcopy

CPP_FLAGS= -mcpu=cortex-m0plus \
	-mthumb \
	-c -g -Os -w \
	-std=gnu++11 \
	-ffunction-sections \
	-fdata-sections \
	-fno-threadsafe-statics \
	-nostdlib \
	--param max-inline-insns-single=500 \
	-fno-rtti \
	-fno-exceptions \
	-MMD

CC_FLAGS= -mcpu=cortex-m0plus \
	-mthumb \
	-c -g -Os -w \
	-std=gnu11 \
	-ffunction-sections \
	-fdata-sections \
	-nostdlib \
	--param max-inline-insns-single=500 \
	-MMD

CPP_DEFS= -DF_CPU=48000000L \
	-DARDUINO=10609 \
	-DARDUINO_SAMD_FEATHER_M0 \
	-DARDUINO_ARCH_SAMD \
	-DARDUINO_SAMD_ZERO \
	-D__SAMD21G18A__ \
	-DUSB_VID=0x239A \
	-DUSB_PID=0x800B \
	-DUSBCON \
	'-DUSB_MANUFACTURER="Adafruit"' \
	'-DUSB_PRODUCT="Feather M0"' \
	$(PKG_CPP_DEFS)

CC_DEFS= $(CPP_DEFS)

PKG_INCS=$(addprefix -I$(ROOT)/,$(PACKAGES))
CPKG_INCS=-I$(ROOT)/Adafruit_BluefruitLE_nRF51 -I$(ROOT)/Adafruit_Motorshield
HPKG_INCS=$(addprefix -I$(ROOT)/,$(addsuffix /src,$(HPACKAGES)))

CPP_INCS=-I$(ARDUINO_TOOLS)/CMSIS/4.0.0-atmel/CMSIS/Include/ \
	-I$(ARDUINO_TOOLS)/CMSIS/4.0.0-atmel/Device/ATMEL/ \
	-I$(ARDUINO_TOOLS)/CMSIS/4.0.0-atmel/Device/ATMEL/$(DFAMILY)/include/component/ \
	-I$(ADAFRUIT_TOOLS)/cores/arduino \
	-I$(ADAFRUIT_TOOLS)/variants/arduino_zero \
	-I$(ADAFRUIT_TOOLS)/libraries/SPI \
	$(PKG_INCS) \
	$(CPKG_INCS) \
	$(HPKG_INCS) \
	-I$(ROOT)/Adafruit_Motorshield/utility \
	-I$(ROOT)

CC_INCS=-I$(ARDUINO_TOOLS)/CMSIS/4.0.0-atmel/CMSIS/Include/ \
	-I$(ARDUINO_TOOLS)/CMSIS/4.0.0-atmel/Device/ATMEL/ \
	-I$(ADAFRUIT_TOOLS)/cores/arduino \
	-I$(ADAFRUIT_TOOLS)/variants/arduino_zero \
	$(CPKG_INCS)


HEADERS=$(wildcard $(addsuffix *.h,$(addprefix $(ROOT),$(PACKAGES))))

all: $(SKETCH).bin
	@echo all done

beecnt: beecnt.t.bin
	@echo done beecnt

latch: latch.t.bin
	@echo done latch

audio: audio.t.bin
	@echo done audio

$(SKETCH).bin: $(SKETCH).elf
	$(OBJCPY) -O binary $(SKETCH).elf $@

beecnt.t.bin: beecnt.t.elf
	$(OBJCPY) -O binary $< $@

latch.t.bin: latch.t.elf
	$(OBJCPY) -O binary $< $@

audio.t.bin: audio.t.elf
	$(OBJCPY) -O binary $< $@

$(SKETCH).elf: objs
	@echo $(PACKAGE_LIBS)
	$(CC) -Os -Wl,--gc-sections -save-temps  -T$(LD_SCRIPT) "-Wl,-Map,$(SKETCH).map" --specs=nano.specs --specs=nosys.specs -mcpu=cortex-m0plus -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -o $@ $(SKETCH).o -Wl,--start-group -lm $(ROOT)/app.a -Wl,--end-group

beecnt.t.elf: objs
	@echo $(PACKAGE_LIBS)
	$(CC) -Os -Wl,--gc-sections -save-temps  -T$(LD_SCRIPT) "-Wl,-Map,beecnt.t.map" --specs=nano.specs --specs=nosys.specs -mcpu=cortex-m0plus -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -o $@ beecnt.t.o -Wl,--start-group -lm $(ROOT)/app.a -Wl,--end-group

latch.t.elf: objs
	@echo $(PACKAGE_LIBS)
	$(CC) -Os -Wl,--gc-sections -save-temps  -T$(LD_SCRIPT) "-Wl,-Map,latch.t.map" --specs=nano.specs --specs=nosys.specs -mcpu=cortex-m0plus -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -o $@ latch.t.o -Wl,--start-group -lm $(ROOT)/app.a -Wl,--end-group

audio.t.elf: objs
	@echo $(PACKAGE_LIBS)
	$(CC) -Os -Wl,--gc-sections -save-temps  -T$(LD_SCRIPT) "-Wl,-Map,audio.t.map" --specs=nano.specs --specs=nosys.specs -mcpu=cortex-m0plus -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -o $@ audio.t.o -Wl,--start-group -lm $(ROOT)/app.a -Wl,--end-group


objs: $(SKETCH).o $(LOCALOBJS) $(PACKAGE_LIBS) beecnt.t.o latch.t.o audio.t.o
	@echo PACKAGE_LIBS == $(PACKAGE_LIBS)
	@echo OBJS done

$(SKETCH).o : $(SKETCH).cpp
	$(ROOT)/setbuild.bsh > /tmp/version_id.h
	cp /tmp/version_id.h $(ROOT)/version_id.h
	$(CPP) $(CPP_FLAGS) $(CPP_DEFS) $(CPP_INCS) $< -o $@

beecnt.t.o : beecnt.t.cpp
	$(CPP) $(CPP_FLAGS) $(CPP_DEFS) $(CPP_INCS) $< -o $@

latch.t.o : latch.t.cpp
	$(CPP) $(CPP_FLAGS) $(CPP_DEFS) $(CPP_INCS) $< -o $@

audio.t.o : audio.t.cpp
	$(CPP) $(CPP_FLAGS) $(CPP_DEFS) $(CPP_INCS) $< -o $@

%.a : $(patsubst %.cpp,%.o,$(wildcard *.cpp)) $(patsubst %.c,%.o,$(wildcard *.c))
	@echo OBJS: $(patsubst %.cpp,%.o,$(wildcard *.cpp)) $(patsubst %.c,%.o,$(wildcard *.c))
	@echo $(subst .a,,$@) is ready

.PRECIOUS : $(patsubst %.cpp,%.o,$(wildcard *.cpp)) $(patsubst %.c,%.o,$(wildcard *.c))

%.o : %.cpp $(HEADERS)
	$(CPP) $(CPP_FLAGS) $(CPP_DEFS) $(CPP_INCS) $< -o $@
	$(AR) rcs $(ROOT)/app.a $@

%.o : %.c $(HEADERS)
	$(CC) $(CC_FLAGS) $(CC_DEFS) $(CC_INCS) $< -o $@
	$(AR) rcs $(ROOT)/app.a $@


.PHONY: $(PACKAGE)

%.lib :
	@echo Making lib $@
	echo "cd $(subst .lib,,$@); make -f $(ROOT)/Makefile $(subst .lib,.a,$@)"
	cd $(subst .lib,,$@); make -f $(ROOT)/Makefile $(subst .lib,.a,$@)

Adafruit_BluefruitLE_nRF51.lib :
	cd Adafruit_BluefruitLE_nRF51; make -f $(ROOT)/Makefile Adafruit_BluefruitLE_nRF51.a
	cd Adafruit_BluefruitLE_nRF51/utility; make -f $(ROOT)/Makefile Adafruit_BluefruitLE_nRF51.a

Adafruit_Motorshield.lib : Adafruit_Motorshield/Adafruit_Motorshield.a
	@echo Adafruit_Motorshield.lib done

Motorshield.root = $(ROOT)/Adafruit_Motorshield
Motorshield.util = $(Motorshield.root)/utility

Motorshield.root.srcs = $(wildcard $(Motorshield.root)/*.cpp) $(wildcard $(Motorshield.root)/*.c)
Motorshield.util.srcs = $(wildcard $(Motorshield.util)/*.cpp) $(wildcard $(Motorshield.util)/*.c)

Adafruit_Motorshield/Adafruit_Motorshield.a: Adafruit_Motorshield/Motorshield.root.a \
					     Adafruit_Motorshield/Motorshield.util.a
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(Motorshield.root.srcs)) $(patsubst %.c,%.o,$(Motorshield.root.srcs))
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(Motorshield.util.srcs)) $(patsubst %.c,%.o,$(Motorshield.util.srcs))

Adafruit_Motorshield/Motorshield.root.a : $(Motorshield.root.srcs)
	cd $(Motorshield.root); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(Motorshield.root)/*.o)

Adafruit_Motorshield/Motorshield.util.a : $(Motorshield.util.srcs)
	cd $(Motorshield.util); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(Motorshield.util)/*.o)

lib.objs : $(LIBOBJS)
	@echo LIBOBJS: $(LIBOBJS)
	@echo $@ is ready

clean :
	@echo Deleting: && find -L $(ROOT) -name "*.o" 
	find -L $(ROOT) -name "*.o" | xargs rm -f
	@echo Deleting: && find -L $(ROOT) -name "*.d" 
	find -L $(ROOT) -name "*.d" | xargs rm -f
	@echo Deleting: && find -L $(ROOT) -name "*.a" 
	find -L $(ROOT) -name "*.a" | xargs rm -f
	rm $(SKETCH).map $(SKETCH).elf $(SKETCH).bin

myclean :
	@echo Deleting: && find -L $(addprefix $(ROOT),$(MYPACKAGES)) -name "*.o"
	find -L $(addprefix $(ROOT),$(MYPACKAGES)) -name "*.o" | xargs rm -f
