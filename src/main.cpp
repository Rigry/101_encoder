#define STM32F030x6
#define F_CPU   48000000UL

#include "periph_rcc.h"
#include "flash.h"
#include "pin.h"
#include "buttons.h"
#include "modbus_master.h"
#include "literals.h"
#include "init_clock.h"
#include "control_move.h"

/// эта функция вызываеться первой в startup файле
extern "C" void init_clock () { init_clock<8_MHz,F_CPU>(); }

using DI1 = mcu::PA9;  using Encoder_b = DI1;
using DI2 = mcu::PA8;  using Encoder_a = DI2;
using DI3 = mcu::PB15; using Sensor_origin = DI3;
using DI4 = mcu::PB14; using Sensor_right  = DI4;

using DO1 = mcu::PB0; 
using DO2 = mcu::PA7; 
using DO3 = mcu::PA6; 
using DO4 = mcu::PA5; 

using RX  = mcu::PA3;
using TX  = mcu::PA2;
using RTS = mcu::PA1;

int main()
{
   auto [sensor_origin, sensor_right] = make_pins<mcu::PinMode::Input,DI3,DI4>();

   Flash<Flash_data, mcu::FLASH::Sector::_8> flash{};

   decltype(auto) modbus = Modbus_slave<In_regs, Out_regs>
                 ::make<mcu::Periph::USART1, TX, RX, RTS>
                       (flash.modbus_address, flash.uart_set);

   decltype(auto) encoder = Encoder::make<mcu::Periph::TIM1, Encoder_a, Encoder_b>();

   modbus.outRegs.device_code       = 10;
   modbus.outRegs.factory_number    = flash.factory_number;
   modbus.outRegs.modbus_address    = flash.modbus_address;
   modbus.outRegs.uart_set          = flash.uart_set;
   modbus.arInRegsMax[ADR(uart_set)]= 0b11111111;
   modbus.inRegsMin.modbus_address  = 1;
   modbus.inRegsMax.modbus_address  = 255;

   using Flash  = decltype(flash);
   using Modbus = Modbus_slave<In_regs, Out_regs>;

   Control_move<Flash, Modbus> control_move {sensor_origin, sensor_right, encoder, flash, modbus}; 

   while(1){
      control_move();
      __WFI();
   }

}

