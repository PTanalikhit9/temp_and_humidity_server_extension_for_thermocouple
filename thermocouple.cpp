
#include "mbed.h"
#include "MAX31856.h"


// Hardware serial port over USB micro
Serial serial(USBTX, USBRX);

//SPI spi(SPIO MOSI,SPIO MISO,SPIO SCK);
SPI spi(P2_1, P2_2, P2_0);
//----------------------------------------------------------

//Thermocouples
MAX31856 Thermocouple(spi, P2_3);
Thermocouple.setThermocoupleType(MAX31856_TCTYPE_T);

DigitalOut led1(LED1);

int main() {

    
    float temperature_TC, temperature_CJ;
    
    while (true) {
        led1 = !led1;
        temperature_TC=Thermocouple.readTC();
        temperature_CJ=Thermocouple.readCJ();

        wait(0.2);

        serial.printf("MAX31856.CJ = %f C   MAX31856.TC = %f C\n\r",temperature_CJ,temperature_TC);  
    }
}

