#pragma once

#include "pin.h"
#include "encoder.h"
#include "flash.h"
#include "modbus_slave.h"

struct Sensors {
   bool sensor_origin:1;  // Bit 0 sensor_origin: Origin Sensor
   bool sensor_right :1;  // Bit 1 sensor_right: Right Sensor
   uint16_t res      :14; // Bits 15:2 res: Reserved, must be kept cleared
}__attribute__((packed));;

struct In_regs {
   
   UART::Settings uart_set;         // 0
   uint16_t modbus_address;         // 1
   uint16_t password;               // 2
   uint16_t factory_number;         // 3

}__attribute__((packed));

struct Out_regs {

   uint16_t device_code;            // 0
   uint16_t factory_number;         // 1
   UART::Settings uart_set;         // 2
   uint16_t modbus_address;         // 3
   int16_t max_coordinate;          // 4
   int16_t coordinate;              // 5
   Sensors sensors;                 // 6

}__attribute__((packed));

struct Flash_data {
   uint16_t factory_number = 0;
   UART::Settings uart_set = {
      .parity_enable  = false,
      .parity         = USART::Parity::even,
      .data_bits      = USART::DataBits::_8,
      .stop_bits      = USART::StopBits::_1,
      .baudrate       = USART::Baudrate::BR9600,
      .res            = 0
   };
   uint8_t  modbus_address = 1;
   uint16_t model_number   = 0;
};

#define ADR(reg) GET_ADR(In_regs, reg)

template<class Flash, class Modbus>
class Control_move
{
   Pin& sensor_origin;
   Pin& sensor_right;
   Encoder& encoder;
   Flash& flash;
   Modbus& modbus;

public:
   Control_move(Pin& sensor_origin, Pin& sensor_right, Encoder& encoder, Flash& flash, Modbus& modbus) 
      : sensor_origin{sensor_origin}
      , sensor_right{sensor_right}
      , encoder {encoder}
      , flash {flash}
      , modbus {modbus}
   {}

   void operator()() {

      encoder = sensor_origin ? 0 : encoder;
      modbus.outRegs.max_coordinate = sensor_right ? encoder : modbus.outRegs.max_coordinate;

      modbus.outRegs.coordinate = encoder;
      modbus.outRegs.sensors.sensor_origin = sensor_origin;
      modbus.outRegs.sensors.sensor_right  = sensor_right;

      modbus([&](uint16_t registrAddress) {
            static bool unblock = false;
         switch (registrAddress) {
            case ADR(uart_set):
               flash.uart_set
                  = modbus.outRegs.uart_set
                  = modbus.inRegs.uart_set;
            break;
            case ADR(modbus_address):
               flash.modbus_address 
                  = modbus.outRegs.modbus_address
                  = modbus.inRegs.modbus_address;
            break;
            case ADR(password):
               unblock = modbus.inRegs.password == 208;
            break;
            case ADR(factory_number):
               if (unblock) {
                  unblock = false;
                  flash.factory_number 
                     = modbus.outRegs.factory_number
                     = modbus.inRegs.factory_number;
               }
               unblock = true;
            break;
         } // switch
      });
   }//operator
};