#include <ModbusSlave.h>

#include <Wire.h>
//this requires SCL->A5 and SDA->A4
#include <hd44780.h>
#undef hd44780_h // undefine this so the example sketch does not think hd44780 is being used.
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd;


#define SLAVE_ID 1           // The Modbus slave ID, change to the ID you want to use.
#define SERIAL_BAUDRATE 9600 // Change to the baudrate you want to use for Modbus communication.
#define SERIAL_PORT Serial   // Serial port to use for RS485 communication, change to the port you're using.

// Comment out the following line if your not using RS485
#define RS485_CTRL_PIN 4 // Change to the pin the RE/DE pin of the RS485 controller is connected to.

// The position in the array determines the address. Position 0 will correspond to Coil, Discrete input or Input register 0.
uint8_t input_pins[] = {2, 3};     // Add the pins you want to read as a Discrete input.
uint8_t output_pins[] = {8};   // Add the pins you want to control via a Coil.
uint8_t analog_pins[] = {A2, A3}; // Add the pins you want to read as a Input register.

uint8_t input_pins_size = sizeof(input_pins) / sizeof(input_pins[0]);    // Get the size of the input_pins array
uint8_t output_pins_size = sizeof(output_pins) / sizeof(output_pins[0]); // Get the size of the output_pins array
uint8_t analog_pins_size = sizeof(analog_pins) / sizeof(analog_pins[0]); // Get the size of the analog_pins array

#ifdef RS485_CTRL_PIN
// Modbus object declaration
Modbus slave(SERIAL_PORT, SLAVE_ID, RS485_CTRL_PIN);
#else
Modbus slave(SERIAL_PORT, SLAVE_ID);
#endif

#define LED_1 11
#define LED_2 12

uint16_t memory_slave[] = {0, 0, 0, 1, 3, 0, 0, 7, 8, 9};
uint8_t memory_slave_size = sizeof(memory_slave) / sizeof(memory_slave[0]); // Get the size of the input_pins array

void setup()
{
    lcd.init();
    
    lcd.begin(16,2);
    
    lcd.setCursor(0,0);
    lcd.print("Modbus Slave/LGT8");
    lcd.setCursor(0,1);
    lcd.print("by TienTN");
    
    delay(2000);
   
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);

    lcd.setCursor(0,1);
    lcd.print("Testing LED1");

    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_1, HIGH);
      delay(300);
      digitalWrite(LED_1, LOW);
      delay(300);
    }

    lcd.setCursor(0,1);
    lcd.print("Testing LED2");

    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_2, HIGH);
      delay(300);
      digitalWrite(LED_2, LOW);
      delay(300);
    }

    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);

    lcd.setCursor(0,1);
    lcd.print("Testing LEDs");

    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_1, HIGH);
      delay(200);
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, HIGH);
      delay(200);
      digitalWrite(LED_2, LOW);
      delay(200);
    }

    lcd.setCursor(0,1);
    lcd.print("Setting registers");

    // Set the defined input pins to input mode.
    for (int i = 0; i < input_pins_size; i++)
    {
        pinMode(input_pins[i], INPUT);
    }

    // Set the defined analog pins to input mode.
    for (int i = 0; i < analog_pins_size; i++)
    {
        pinMode(analog_pins[i], INPUT);
    }

    // Set the defined output pins to output mode.
    for (int i = 0; i < output_pins_size; i++)
    {
        pinMode(output_pins[i], OUTPUT);
    }

    // Register functions to call when a certain function code is received.
    slave.cbVector[CB_WRITE_COILS] = writeDigitalOut;
    slave.cbVector[CB_READ_DISCRETE_INPUTS] = readDigitalIn;
    slave.cbVector[CB_READ_INPUT_REGISTERS] = readAnalogIn;
    slave.cbVector[CB_WRITE_HOLDING_REGISTERS] = writeHoldingRegisters; //FC6/16
    slave.cbVector[CB_READ_HOLDING_REGISTERS] = readHoldingRegisters; //FC3

    // Set the serial port and slave to the given baudrate.
    SERIAL_PORT.begin(SERIAL_BAUDRATE);
    slave.begin(SERIAL_BAUDRATE);

    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Unit Ready!");

}

float inCurrent = 423.69;
float inVoltage = 243.31;

void pollSensors() {

   uint16_t inCurrentData[2]; //uint16 is 16bit/2bytes, float is 32bit/4 bytes
   memcpy(inCurrentData, &inCurrent, sizeof(inCurrentData));
   uint16_t inVoltageData[2]; //uint16 is 16bit/2bytes, float is 32bit/4 bytes
   memcpy(inVoltageData, &inVoltage, sizeof(inVoltageData));

   //save to registers
   memory_slave[3] = inCurrentData[0];
   memory_slave[4] = inCurrentData[1];
   memory_slave[5] = inVoltageData[0];
   memory_slave[6] = inVoltageData[1];

}

void loop()
{
    pollSensors();

    // Listen for modbus requests on the serial port.
    // When a request is received it's going to get validated.
    // And if there is a function registered to the received function code, this function will be executed.
    slave.poll();
}

// Modbus handler functions
// The handler functions must return an uint8_t and take the following parameters:
//     uint8_t  fc - function code
//     uint16_t address - first register/coil address
//     uint16_t length/status - length of data / coil status

// Handle the function codes Force Single Coil (FC=05) and Force Multiple Coils (FC=15) and set the corresponding digital output pins (coils).
uint8_t writeDigitalOut(uint8_t fc, uint16_t address, uint16_t length)
{
    // Check if the requested addresses exist in the array
    if (address > output_pins_size || (address + length) > output_pins_size)
    {
        return STATUS_ILLEGAL_DATA_ADDRESS;
    }

    // Set the output pins to the given state.
    for (int i = 0; i < length; i++)
    {
        // Write the value in the input buffer to the digital pin.
        digitalWrite(output_pins[address + i], slave.readCoilFromBuffer(i));
    }

    return STATUS_OK;
}

// Handle the function code Read Input Status (FC=02) and write back the values from the digital input pins (discreet input).
uint8_t readDigitalIn(uint8_t fc, uint16_t address, uint16_t length)
{
    // Check if the requested addresses exist in the array
    if (address > input_pins_size || (address + length) > input_pins_size)
    {
        return STATUS_ILLEGAL_DATA_ADDRESS;
    }

    // Read the digital inputs.
    for (int i = 0; i < length; i++)
    {
        // Write the state of the digital pin to the response buffer.
        slave.writeCoilToBuffer(i, digitalRead(input_pins[address + i]));
    }

    return STATUS_OK;
}

// Handle the function code Read Input Registers (FC=04) and write back the values from analog input pins (input registers).
uint8_t readAnalogIn(uint8_t fc, uint16_t address, uint16_t length)
{
    // Check if the requested addresses exist in the array
    if (address > analog_pins_size || (address + length) > analog_pins_size)
    {
        return STATUS_ILLEGAL_DATA_ADDRESS;
    }

    // Read the analog inputs
    for (int i = 0; i < length; i++)
    {
        // Write the state of the analog pin to the response buffer.
        slave.writeRegisterToBuffer(i, analogRead(analog_pins[address + i]));
    }

    return STATUS_OK;
}

// Handle the function codes Write Holding Register(s) (FC=06, FC=16)
uint8_t writeHoldingRegisters(uint8_t fc, uint16_t address, uint16_t length)
{
    // Check if the requested addresses exist in the array
    if (address > memory_slave_size || (address + length) > memory_slave_size)
    {
        return STATUS_ILLEGAL_DATA_ADDRESS;
    }

    lcd.setCursor(0,1);
    char myMsg[25];

    // Write the received data into the memory array.
    for (uint8_t i = 0; i < length; ++i)
    {
        uint16_t address_val = address + i;

        uint16_t value = slave.readRegisterFromBuffer(i);

        sprintf(myMsg, "WriteSR %d-%d ", address_val, value);

        if (value == 1) {
          if (address_val == 0)
            digitalWrite(LED_1,HIGH);
          else if (address_val == 1)
            digitalWrite(LED_2,HIGH);
        }
        else {
          if (address_val == 0)
            digitalWrite(LED_1,LOW);
          else if (address_val == 1)
            digitalWrite(LED_2,LOW);
        }
        memory_slave[address_val] = value;
        lcd.print(myMsg);
    }
    return STATUS_OK;
}

// Handle the function code Read Holding Registers (FC=03).
uint8_t readHoldingRegisters(uint8_t fc, uint16_t address, uint16_t length)
{
    // Check if the requested addresses exist in the array.
    if (address > memory_slave_size || (address + length) > memory_slave_size)
    {
        return STATUS_ILLEGAL_DATA_ADDRESS;
    }

    char myMsg[25];

    lcd.setCursor(0,1);

    /*
    float inVoltage = 243.31;
    uint16_t inVoltageData[2]; //uint16 is 16bit/2bytes, float is 32bit/4 bytes
    memcpy(inVoltageData, &inVoltage, sizeof(inVoltageData));
    //save to registers
    memory_slave[0] = inVoltageData[0];
    memory_slave[1] = inVoltageData[1];
    */

    String myMsgO = "ReadSR ";
    myMsgO.concat(address);
    myMsgO.concat("-");
    myMsgO.concat(length);
    myMsgO.concat("-");
    myMsgO.concat(inCurrent); //arduino not support sprintf float
    myMsgO.concat(" ");
    myMsgO.toCharArray(myMsg, myMsgO.length() + 1);

    lcd.print(myMsg);

    // Write the memory array into the send buffer.
    for (uint8_t i = 0; i < length; ++i)
    {
        slave.writeRegisterToBuffer(i, memory_slave[address + i]);
    }
    return STATUS_OK;
}
