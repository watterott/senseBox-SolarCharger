/*
  SolarCharger ATtiny841 Firmware
*/

/*
 * Using ATtinycore, ATtiny 841, No bootloader, 8MHz
 * 
 * 
 * Charge controller for sensebox power module. Functionalities:
 * Set PA3 to output low
 * Set PA2 to input, pullup
 * Set PA0, PA1, to analog input
 * Check temp, if temp above 12*C enable fast charge (PA3 high), 
 * if temp below 8*C disable fast charge (PA3 low)
 * Measure incoming voltage on PA1
 * Set PA7, PB0, PB1, PB2 to output high
 * Measure cell voltage on PA0
 * If cell voltage over 3.2V PA7 low
 * If cell voltage over 3.6V PB0 low
 * If cell voltage over 3.7V PB1 low
 * If cell voltage over 3.9V PB2 low
 * Check status for charge
 * Provide I2C i/f with following info:
 * - Register 0: cell voltage, 20mV/LSB
 * - Register 1: input voltage, 100mV/LSB
 * - Register 2: status bits: [B,I,L3,L2,L1,L0,F,C]
 *    B=battery present >2.8V<4.8V
 *    I=Input voltage present >4.5V
 *    L0-L3=battery status LEDs
 *    F=Fast charge enabled
 *    C=Charging
 * - Register 3: temperature in C, signed 8-bit

*/

#include <Wire.h>
#define SLAVE_ADDRESS 0x32
#define TEMP_OFFSET   275

volatile uint8_t i2creg[]={0x00,0x00,0x00,0x00};
volatile char i2cindex=0;

void setup()
{
  // put your setup code here, to run once:
  pinMode(PA0, INPUT);
  pinMode(PA1, INPUT);
  pinMode(PA2, INPUT);
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(0,HIGH);
  digitalWrite(1,HIGH);
  digitalWrite(2,HIGH);
  digitalWrite(3,HIGH);
  digitalWrite(7,LOW);
  digitalWrite(PA2,HIGH);//pullup
  digitalWrite(PA0,LOW);//no pullup
  digitalWrite(PA1,LOW);//no pullup
  Wire.begin(SLAVE_ADDRESS);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
  
}

void loop()
{
  uint8_t flags=0;
  // put your main code here, to run repeatedly:
  analogReference(INTERNAL1V1);
  analogRead(0x0C);
  delay(10);
  analogRead(0x0C);
  uint32_t temp_raw=analogRead(0x0C);
  int temp=(int)temp_raw-TEMP_OFFSET;
  i2creg[3]=temp;
  // 1LSB/C, calibrate if necessary

  analogReference(DEFAULT);
  analogRead(0x01);
  delay(10);
  int vin_raw=analogRead(0x01);
  //1024 LSBs, 5V VREF, 4.88mV/LSB
  //VIN is a 10k/1.2k voltage divider
  //VIN scaling factor is 9.3
  //vin_raw therefore has 4.88*9.3=45.54mV/LSB
  //we want vin in 100mV increments so we need to multiply by 0.45
  int vin=0.45*vin_raw;
  i2creg[1]=vin;
  if(vin>45){
    flags|=(1<<6);
  }else{
    flags&=~(1<<6);
  }
  
  //quick charge when warm enough and voltage >8V
  if(temp>=12 && temp<46 && vin>80){
    digitalWrite(7,HIGH);
    flags|=(1<<1);
  }
  if(temp<=8 || temp>50 || vin<80){
    digitalWrite(7,LOW);//hysteresis
    flags&=~(1<<1);
  }

  analogRead(0x00);
  uint32_t vbat_raw=analogRead(0x00);
  //1024 LSBs, 5V VREF, 4.88mV/LSB
  //we want vin in 20mV increments so we need to multiply by 0.244
  int vbat=0.244*vbat_raw;
  i2creg[0]=vbat;
  if(vbat>140 && vbat<240){
    flags|=(1<<7);
  }else{
    flags&=~(1<<7);
  }
  
  if(digitalRead(PA2)==LOW && vin>45 && vbat>140 && vbat <240){ //is battery charging - only possible if supply voltage high enough and battery present
    flags|=(1);
  }else{
    flags&=~(1);
  }

  if(vbat>160){
    digitalWrite(3,LOW);
    flags|=(1<<(2)); 
  }else{
    digitalWrite(3,HIGH);
    flags&=~(1<<(2)); 
  }
  if(vbat>180){
    digitalWrite(0,LOW);
    flags|=(1<<(3)); 
  }else{
    digitalWrite(0,HIGH);
    flags&=~(1<<(3)); 
  }
  if(vbat>185){
    digitalWrite(1,LOW);
    flags|=(1<<(4)); 
  }else{
    digitalWrite(1,HIGH);
    flags&=~(1<<(4)); 
  }
  if(vbat>195){
    digitalWrite(2,LOW);
    flags|=(1<<(5)); 
  }else{
    digitalWrite(2,HIGH);
    flags&=~(1<<(5)); 
  }
  i2creg[2]=flags;
  delay(100);
  
}

void receiveEvent()
{
  if(Wire.available()){
    i2cindex=Wire.read();
  }
}

void requestEvent()
{
  if(i2cindex>3)i2cindex=0;
  Wire.write(i2creg+i2cindex, 4-i2cindex);
  i2cindex=0;
}
