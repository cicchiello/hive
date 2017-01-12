ROOT=/home/pi/hive/feather/src
SKETCH=hive

LOCALOBJS=CpuTempSensor.o Parse.o Timestamp.o Sensor.o Actuator.o \
	HumidSensor.o TempSensor.o \
	StepperMonitor.o StepperActuator.o \
	SensorRateActuator.o
MYPACKAGES=strutils couchutils tests platformutils cloudpipe DHT
OTHERPACKAGES=SPI arduino arduino_zero USB Wire
PACKAGES=$(MYPACKAGES) $(OTHERPACKAGES)

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

$(SKETCH).bin: $(SKETCH).elf
	$(OBJCPY) -O binary $(SKETCH).elf $@

$(SKETCH).elf: objs
	@echo $(PACKAGE_LIBS)
	$(CC) -Os -Wl,--gc-sections -save-temps  -T$(LD_SCRIPT) "-Wl,-Map,$(SKETCH).map" --specs=nano.specs --specs=nosys.specs -mcpu=cortex-m0plus -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -o $@ $(SKETCH).o -Wl,--start-group -lm $(ROOT)/app.a -Wl,--end-group


objs: $(SKETCH).o $(LOCALOBJS) $(PACKAGE_LIBS)
	@echo PACKAGE_LIBS == $(PACKAGE_LIBS)
	@echo OBJS done

$(SKETCH).o : $(SKETCH).cpp
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

Adafruit_Motorshield.lib :
	cd Adafruit_Motorshield; make -f $(ROOT)/Makefile Adafruit_Motorshield.a
	cd Adafruit_Motorshield/utility; make -f $(ROOT)/Makefile Adafruit_Motorshield.a

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
