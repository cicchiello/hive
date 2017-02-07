ROOT := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

APP=hive

TESTS=feather blink beecnt latch audio

DFAMILY=samd21

TESTBINS=$(addsuffix .t.bin,$(TESTS))
TESTOBJS=$(addsuffix .t.o,$(TESTS))
LOCALOBJS= \
        Timestamp.o Sensor.o Actuator.o Heartbeat.o \
	hive_platform.o Trace.o \
	SensorRateActuator.o TempSensor.o HumidSensor.o \
	ConfigReader.o ConfigWriter.o hiveconfig.o \
	SensorBase.o StepperMonitor.o ActuatorBase.o StepperActuator.o \
	beecnt.o latch.o AudioActuator.o sine.o

#	ServoConfigActuator.o \

MYPACKAGES=strutils couchutils platformutils DHT tests wifiutils cloudpipe \
	httputils adcutils sdutils listener rtc dacutils
OTHERPACKAGES=SdFat SPI arduino arduino_zero USB Wire
PACKAGES=$(MYPACKAGES) $(OTHERPACKAGES)

LIBOBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp)) $(patsubst %.c,%.o,$(wildcard *.c))

HPACKAGES=WiFi101
CPACKAGES=Adafruit_Motorshield
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
	-I$(ROOT)/WiFi101/src \
	$(CPKG_INCS)


HEADERS=$(wildcard $(addsuffix *.h,$(addprefix $(ROOT),$(PACKAGES))))

all: $(APP).bin $(TESTBINS)
	@echo all done

$(APP).bin: $(APP).elf
	$(OBJCPY) -O binary $(APP).elf $@

%.t.bin: %.t.elf
	$(OBJCPY) -O binary $< $@

$(APP).elf: $(APP).o local.a $(PACKAGE_LIBS)
	@echo PACKAGE_LIBS: $(PACKAGE_LIBS)
	$(CC) -Os -Wl,--gc-sections -save-temps  -T$(LD_SCRIPT) "-Wl,-Map,$(APP).map" --specs=nano.specs --specs=nosys.specs -mcpu=cortex-m0plus -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -o $@ $(APP).o -Wl,--start-group -lm local.a $(foreach lib,$(patsubst %.lib,%,$(PACKAGE_LIBS)),-lm $(lib)/$(lib).a) -Wl,--end-group

%.t.elf: %.t.o local.a $(PACKAGE_LIBS)
	@echo $(foreach lib,$(patsubst %.lib,%,$(PACKAGE_LIBS)),$(lib)/$(lib).a)
	$(CC) -Os -Wl,--gc-sections -save-temps  -T$(LD_SCRIPT) "-Wl,-Map,$(APP).map" --specs=nano.specs --specs=nosys.specs -mcpu=cortex-m0plus -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -o $@ $< -Wl,--start-group -lm local.a $(foreach lib,$(patsubst %.lib,%,$(PACKAGE_LIBS)),-lm $(lib)/$(lib).a) -Wl,--end-group

objs: $(APP).o $(LOCALOBJS) $(TESTOBJS) $(PACKAGE_LIBS) 
	@echo PACKAGE_LIBS == $(PACKAGE_LIBS)
	@echo OBJS done

$(APP).o : $(APP).cpp
	$(ROOT)/setbuild.bsh > /tmp/version_id.h
	cp /tmp/version_id.h $(ROOT)/version_id.h
	$(CPP) $(CPP_FLAGS) $(CPP_DEFS) $(CPP_INCS) $< -o $@

local.a : $(LOCALOBJS)
	@echo OBJS for $@: $(LOCALOBJS)
	$(AR) rcs $@ $(LOCALOBJS)
	@echo $@ is ready

lib.objs : $(LIBOBJS)
	@echo LIBOBJS: $(LIBOBJS)
	@echo $@ is ready

%.a : $(LIBOBJS)
	@echo OBJS for $@: $(LIBOBJS)
	$(AR) rcs $@ $(LIBOBJS)
	@echo $@ is ready

.PRECIOUS : $(LIBOBJS)

%.o : %.cpp $(HEADERS)
	$(CPP) $(CPP_FLAGS) $(CPP_DEFS) $(CPP_INCS) $< -o $@

%.t.o : %.t.cpp $(HEADERS)
	$(CPP) $(CPP_FLAGS) $(CPP_DEFS) $(CPP_INCS) $< -o $@

%.o : %.c $(HEADERS)
	$(CC) $(CC_FLAGS) $(CC_DEFS) $(CC_INCS) $< -o $@


.PHONY: $(PACKAGES) lib.objs

%.lib :
	@echo Making lib $@
	echo "cd $(subst .lib,,$@); make -f $(ROOT)/Makefile $(subst .lib,.a,$@)"
	cd $(subst .lib,,$@); make -f $(ROOT)/Makefile $(subst .lib,.a,$@)

WiFi101.lib : WiFi101/WiFi101.a
	@echo WiFi101.lib done

WiFi.root = $(ROOT)/WiFi101/src
WiFiDriver.root = $(WiFi.root)/driver/source
WiFiSocket.root = $(WiFi.root)/socket/source
WiFiCommon.root = $(WiFi.root)/common/source
WiFiBsp.root = $(WiFi.root)/bsp/source
WiFiSpiFlash.root = $(WiFi.root)/spi_flash/source
WiFiBusWrapper.root = $(WiFi.root)/bus_wrapper/source

WiFi.srcs = $(wildcard $(WiFi.root)/*.cpp) $(wildcard $(WiFi.root)/*.c)
WiFiDriver.srcs = $(wildcard $(WiFiDriver.root)/*.cpp) $(wildcard $(WiFiDriver.root)/*.c)
WiFiSocket.srcs = $(wildcard $(WiFiSocket.root)/*.cpp) $(wildcard $(WiFiSocket.root)/*.c)
WiFiCommon.srcs = $(wildcard $(WiFiCommon.root)/*.cpp) $(wildcard $(WiFiCommon.root)/*.c)
WiFiBsp.srcs = $(wildcard $(WiFiBsp.root)/*.cpp) $(wildcard $(WiFiBsp.root)/*.c)
WiFiSpiFlash.srcs = $(wildcard $(WiFiSpiFlash.root)/*.cpp) $(wildcard $(WiFiSpiFlash.root)/*.c)
WiFiBusWrapper.srcs = $(wildcard $(WiFiBusWrapper.root)/*.cpp) $(wildcard $(WiFiBusWrapper.root)/*.c)

WiFi101/WiFi101.a : WiFi101/WiFi101.root.a WiFi101/WiFi101.driver.a WiFi101/WiFi101.socket.a \
		WiFi101/WiFi101.common.a WiFi101/WiFi101.bsp.a WiFi101/WiFi101.spi_flash.a \
		WiFi101/WiFi101.bus_wrapper.a
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(WiFi.srcs)) $(patsubst %.c,%.o,$(WiFi.srcs))
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(WiFiDriver.srcs)) $(patsubst %.c,%.o,$(WiFiDriver.srcs))
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(WiFiSocket.srcs)) $(patsubst %.c,%.o,$(WiFiSocket.srcs))
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(WiFiCommon.srcs)) $(patsubst %.c,%.o,$(WiFiCommon.srcs))
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(WiFiBsp.srcs)) $(patsubst %.c,%.o,$(WiFiBsp.srcs))
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(WiFiSpiFlash.srcs)) $(patsubst %.c,%.o,$(WiFiSpiFlash.srcs))
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(WiFiBusWrapper.srcs)) $(patsubst %.c,%.o,$(WiFiBusWrapper.srcs))

WiFi101/WiFi101.root.a : $(WiFi.srcs)
	cd $(WiFi.root); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(WiFi.root)/*.o)
	@echo $@ done

WiFi101/WiFi101.driver.a : $(WiFiDriver.srcs)
	cd $(WiFiDriver.root); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(WiFiDriver.root)/*.o)
	@echo $@ done

WiFi101/WiFi101.socket.a : $(WiFiSocket.srcs)
	cd $(WiFiSocket.root); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(WiFiSocket.root)/*.o)
	@echo $@ done

WiFi101/WiFi101.common.a : $(WiFiCommon.srcs)
	cd $(WiFiCommon.root); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(WiFiCommon.root)/*.o)
	@echo $@ done

WiFi101/WiFi101.bsp.a : $(WiFiBsp.srcs)
	cd $(WiFiBsp.root); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(WiFiBsp.root)/*.o)
	@echo $@ done

WiFi101/WiFi101.spi_flash.a : $(WiFiSpiFlash.srcs)
	cd $(WiFiSpiFlash.root); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(WiFiSpiFlash.root)/*.o)
	@echo $@ done

WiFi101/WiFi101.bus_wrapper.a : $(WiFiBusWrapper.srcs)
	cd $(WiFiBusWrapper.root); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(WiFiBusWrapper.root)/*.o)
	@echo $@ done


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


SdFat.lib : SdFat/SdFat.a
	@echo $@ done

SdFat.root = $(ROOT)/SdFat
SdFat.utility = $(ROOT)/SdFat/utility

SdFat.root.srcs = $(wildcard $(SdFat.root)/*.cpp) $(wildcard $(SdFat.root)/*.c)
SdFat.utility.srcs = $(wildcard $(SdFat.utility)/*.cpp) $(wildcard $(SdFat.utility)/*.c)

SdFat/SdFat.a : SdFat/SdFat.root.a SdFat/SdFat.utility.a
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(SdFat.root.srcs)) $(patsubst %.c,%.o,$(sdFat.root.srcs))
	$(AR) rcs $@ $(patsubst %.cpp,%.o,$(SdFat.utility.srcs)) $(patsubst %.c,%.o,$(sdFat.utility.srcs))

SdFat/SdFat.root.a : $(SdFat.root.srcs)
	cd $(SdFat.root); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(SdFat.root)/*.o)

SdFat/SdFat.utility.a : $(SdFat.utility.srcs)
	cd $(SdFat.utility); make -f $(ROOT)/Makefile lib.objs
	$(AR) rcs $@ $(wildcard $(SdFat.utility)/*.o)



clean :
	@echo Deleting: && find -L $(ROOT) -name "*.o" 
	find -L $(ROOT) -name "*.o" | xargs rm -f
	@echo Deleting: && find -L $(ROOT) -name "*.d" 
	find -L $(ROOT) -name "*.d" | xargs rm -f
	@echo Deleting: && find -L $(ROOT) -name "*.a" 
	find -L $(ROOT) -name "*.a" | xargs rm -f
	rm *.map *.elf *.bin

myclean :
	@echo Deleting: && find -L $(addprefix $(ROOT),$(MYPACKAGES)) -name "*.o"
	find -L $(addprefix $(ROOT),$(MYPACKAGES)) -name "*.o" | xargs rm -f
