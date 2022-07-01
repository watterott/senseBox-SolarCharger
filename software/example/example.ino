/*
  SolarCharger

  Scans the I2C Bus for SolarCharger - address 0x32.
*/

/*
 * I2C i/f with following info on address 0x32:
 * - Register 0: cell voltage, 20mV/LSB
 * - Register 1: input voltage, 100mV/LSB
 * - Register 2: status bits: [B,I,L3,L2,L1,L0,F,C]
 *    B=battery present >2.8V
 *    I=Input voltage present >4.5V
 *    L0-L3=battery status LEDs
 *    F=Fast charge enabled
 *    C=Charging
 * - Register 3: temperature in C, signed 8-bit
 * Thresholds: L0: 3.2V, L1: 3.6V, L2: 3.7V, L3: 3.9V
 * Switch to slow charge at 8*C
 * Switch to fast charge at 12*C
*/

#include <Wire.h>
#include <senseBoxIO.h>

void setup()
{
  // init serial library
  Serial.begin(9600);
  while(!Serial); // wait for serial monitor
 
  // power on I2C ports
  senseBoxIO.powerI2C(true);

  // init I2C/Wire library
  Wire.begin();
}

void loop()
{
  byte devices, address;

  Serial.println("Scanning...");

  devices = 0;
  address=0x32;
  
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
 
    if(error == 0)
    {
      devices++;
      Serial.print("Device found at 0x");
      Serial.println(address, HEX);
      
        Wire.requestFrom((uint8_t)address, (uint8_t)4);
        uint vbat=Wire.read();
        uint vin=Wire.read();
        uint flags=Wire.read();
        int temp=(int8_t)(Wire.read());
        Serial.println(String("Vbat:")+(20*vbat)+"mv");
        Serial.println(String("Vin:")+(100*vin)+"mv");
        Serial.print(String("Flags:"));
        Serial.println(flags,BIN);
        if(flags&1){
          Serial.println("Charging");
        }else{
          Serial.println("Not charging");
        }
        if(flags&2){
          Serial.println("Temperature normal");
        }else{
          Serial.println("Temperature cold - slow charge only");
        }
        if(flags&64){
          Serial.println("Input voltage sufficient to charge");
        }else{
          Serial.println("Input voltage too low");
        }
        if(flags&128){
          Serial.println("Battery present");
        }else{
          Serial.println("Battery missing");
        }
        Serial.print("Charge LEDs on: ");
        if(flags&4)Serial.print("1 ");
        if(flags&8)Serial.print("2 ");
        if(flags&16)Serial.print("3 ");
        if(flags&32)Serial.print("4 ");
        if((flags&(4|8|16|32))==0){
          Serial.println("None");
        }else{
          Serial.println();
        }
        
        Serial.println(String("temp:")+(temp)+"*C");
        
      
    }
    else if(error == 4)
    {
      Serial.print("Unknown error at 0x");
      Serial.println(address, HEX);
    }    
  
  
  if(devices == 0)
  {
    Serial.println("No devices found\n");
  }
  else
  {
    Serial.println("done\n");
  }

  delay(2000); // wait 2 seconds for next scan
}
