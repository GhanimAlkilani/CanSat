#include <RH_ASK.h> //Transmitter
#include <dht.h> //Temperature & Humidity
#include <SPI.h>
#include <SD.h>
#include <Wire.h>

#define dht_apin A0 // Analog Pin for Temperature/Humidity Sensor 

#define MPU9250_ADDRESS 0x68

#define GYRO_FULL_SCALE_250_DPS 0x00 
#define GYRO_FULL_SCALE_500_DPS 0x08
#define GYRO_FULL_SCALE_1000_DPS 0x10
#define GYRO_FULL_SCALE_2000_DPS 0x18
 
#define ACC_FULL_SCALE_2_G 0x00 
#define ACC_FULL_SCALE_4_G 0x08
#define ACC_FULL_SCALE_8_G 0x10
#define ACC_FULL_SCALE_16_G 0x18

File Temp , Flame ,Ac_Gy;
dht DHT;
RH_ASK driver;


// This function read Nbytes bytes from I2C device at address Address. 
// Put read bytes starting at register Register in the Data array. 
void I2Cread(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data)
{
  // Set register address
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.endTransmission();
   
  // Read Nbytes
  Wire.requestFrom(Address, Nbytes); 
  uint8_t index=0;
  while (Wire.available())
  Data[index++]=Wire.read();
}
 
 
// Write a byte (Data) in device (Address) at register (Register)
void I2CwriteByte(uint8_t Address, uint8_t Register, uint8_t Data)
{
  // Set register address
  Wire.beginTransmission(Address);
  Wire.write(Register);
  Wire.write(Data);
  Wire.endTransmission();
}


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
   if (!driver.init())
         Serial.println("init failed");

  Serial.print("Initializing SD card...");

  if (!SD.begin(53)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  Serial.println("_______________________________________________");

  Wire.begin();
  // Set accelerometers low pass filter at 5Hz
  I2CwriteByte(MPU9250_ADDRESS,29,0x06);
  // Set gyroscope low pass filter at 5Hz
  I2CwriteByte(MPU9250_ADDRESS,26,0x06);
   
   
  // Configure gyroscope range
  I2CwriteByte(MPU9250_ADDRESS,27,GYRO_FULL_SCALE_1000_DPS);
  // Configure accelerometers range
  I2CwriteByte(MPU9250_ADDRESS,28,ACC_FULL_SCALE_4_G);
  delay(1000);
}

void loop() {
    //Read time 
    unsigned long runMillis= millis();
    unsigned long allSeconds=millis()/1000;
    int runHours= allSeconds/3600;
    int secsRemaining=allSeconds%3600;
    int runMinutes=secsRemaining/60;
    int runSeconds=secsRemaining%60;
  
    // Read accelerometer and gyroscope
    uint8_t Buf[14];
    I2Cread(MPU9250_ADDRESS,0x3B,14,Buf);
     
    // Create 16 bits values from 8 bits data
     
    // Accelerometer
    int16_t ax=-(Buf[0]<<8 | Buf[1]);
    int16_t ay=-(Buf[2]<<8 | Buf[3]);
    int16_t az=Buf[4]<<8 | Buf[5];
     
    // Gyroscope
    int16_t gx=-(Buf[8]<<8 | Buf[9]);
    int16_t gy=-(Buf[10]<<8 | Buf[11]);
    int16_t gz=Buf[12]<<8 | Buf[13];

    //Accelerometer&Gyroscope message for transmitter
     String Accelerometer = "| Accelerometer { X :"+  String (ax,DEC)+" Y :" +String(ay,DEC)+" Z :" + String(az,DEC)+ "}";
     String Gyroscope = "| Gyroscope { X :" +String(gx,DEC)+" Y :" +String(gy,DEC)+" Z :" + String(gz,DEC)+" }";

     //convert message from string to char
     char charMsgAccelerometer[60];
     Accelerometer.toCharArray(charMsgAccelerometer, 60) ;
     //convert message from string to char
     char charMsgGyroscope[60];
     Gyroscope.toCharArray(charMsgGyroscope , 60);

     //send Accelerometer&Gyroscope data to reciver
     driver.send((uint8_t *)charMsgAccelerometer, strlen(charMsgAccelerometer));
     driver.waitPacketSent();
     delay(1500);
     driver.send((uint8_t *)charMsgGyroscope, strlen(charMsgGyroscope));
     driver.waitPacketSent();
     delay(1500);
  
    //read temperature & humidity
    DHT.read11(dht_apin);

     //temperature & humidity message for transmitter
     String msg =  "| humidity = " + String(DHT.humidity) + " %  | temperature = " +String(DHT.temperature) + " C |";
     char charMsgTemp[60];
     msg.toCharArray(charMsgTemp, 60) ;

     //send temperature & humidity data to reciver
     driver.send((uint8_t *)charMsgTemp, strlen(charMsgTemp));
     driver.waitPacketSent();
     delay(1500);

   
    //Read Fire 
    int sensorReading = analogRead(A5);
    String flamcheck = sensorReading>30? "No flame":"there is flame";

    //convert String to char
     char charMsgFlame[60];
     flamcheck.toCharArray(charMsgFlame, 60);

     //send flame data to reciver if there is a flame
    if(flamcheck < 30){
       driver.send((uint8_t *)charMsgFlame, strlen(charMsgFlame));
       driver.waitPacketSent();
       delay(500);
     }
  
    
  Temp = SD.open("Temp.txt", FILE_WRITE); // open "Temp.txt" to write data
  Flame = SD.open("Flame.txt",FILE_WRITE); // open "Flame.txt" to write data
  Ac_Gy = SD.open("Ac_Gy.txt",FILE_WRITE); // open "Ac_Gy.txt" to write data
  
  if (Temp && Flame) {
    // write number to file 
    Temp.print("Reading at: |"); 
    Flame.print("Reading at: |"); 
    Ac_Gy.print("Reading at: |");
    
    // Writing time
    char buf[21];
    sprintf(buf,"%02d:%02d:%02d",runHours,runMinutes,runSeconds);
    Temp.print(buf);
    Flame.print(buf);
    Ac_Gy.print(buf);

    //Writing accelerometer and gyroscope
    Ac_Gy.print("| Accelerometer\t X :");
    Ac_Gy.print (ax,DEC); 
    Ac_Gy.print ("  Y :");
    Ac_Gy.print (ay,DEC);
    Ac_Gy.print ("  Z :");
    Ac_Gy.print (az,DEC); 
    Ac_Gy.print ("\t");

    Ac_Gy.print ("Gyroscope\t X :");
    Ac_Gy.print (gx,DEC); 
    Ac_Gy.print ("  Y :");
    Ac_Gy.print (gy,DEC);
    Ac_Gy.print ("  Z :");
    Ac_Gy.print (gz,DEC); 
    Ac_Gy.println("");
    
    //Writing flame check
    Flame.print("| "+flamcheck);
    Flame.println(" | ");

    //Writing Temperature/Humidity check
    Temp.print("| humidity = ");
    Temp.print(DHT.humidity);
    Temp.print(" %  | temperature = ");
    Temp.print(DHT.temperature);
    Temp.println(" C |");

    delay(100);
    
    Ac_Gy.close(); //close Ac_Gy file
    Temp.close(); // close Temp file
    Flame.close(); // close Flame file
  } else {
    Serial.println("Could not open file (writing).");
  }
  
  delay(1000); // wait for 1000ms
}
