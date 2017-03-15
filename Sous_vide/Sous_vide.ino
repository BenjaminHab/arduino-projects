#include <SPI.h>
#include <SD.h>
#include <Adafruit_MAX31865.h>

// Set the pins used
Adafruit_MAX31865 max = Adafruit_MAX31865(9, 10, 12, 13);

#define cardSelect 4
#define RREF 430.0
#define relaypin 11

//variables
char filename[15];
File logfile;
float y,e;
int w=60, u=0;
//integral
float I;
//controller parameters
float K_P=26;
float K_I=0.01;
//stepwidth in ms
int T=1000;

unsigned long t=millis()+1000;
uint8_t j=0;

//declare functions
int PID(float e,float T){
  I+=e*T;
  u=int(e*(K_I*I+K_P));
  //constrain u to range of [0,255]
  if(u>255){u=255;}
  else if(u<0){u=0;}
  return u;
}

//setup
void setup() {
  Serial.begin(115200);
  //while(!Serial){};
  Serial.println("\r\nAnalog logger test");
  //pinMode(13, OUTPUT);


  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect)) {
    Serial.println("Card init. failed!");
    //error(2);
  }

  strcpy(filename, "ANALOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create ");
    Serial.println(filename);
    //error(3);
  }
  Serial.print("Writing to ");
  Serial.println(filename);

  pinMode(13, OUTPUT);
  pinMode(8, OUTPUT);
  Serial.println("Ready!");

  //initialize temperature sensor


  Serial.println("Adafruit MAX31865 PT100 Sensor Test!");
  max.begin(MAX31865_2WIRE);
  //setup output pin
  pinMode(relaypin,OUTPUT);


}



void loop() {
  if(t<millis())
  { t=millis()+T;
    digitalWrite(8, HIGH);
    y=max.temperature(100, RREF);
    e=w-y;
    u=PID(e,T);
    logfile.print(millis()); logfile.print(", "); logfile.print(u); logfile.print(", "); logfile.println(y);
    Serial.print(millis()); Serial.print(", "); Serial.print(u); Serial.print(", "); Serial.println(y);
    //write
    digitalWrite(8, LOW);
    //SSR-Version:
    //analogWrite(relaypin,u);
    //normal Relay version:
    if(u){
      digitalWrite(relaypin,HIGH);
    }
    //write changes to card
    if(j>=100)
    {logfile.flush();
      /*if(!logfile.flush()){ //check, if an error occurs
      logfile.close();
      if(!SD.begin(cardSelect)){
        //if an error occurs on reopening the sdcard, light both leds and shutdown cooker
        digitalWrite(8,HIGH);
        digitalWrite(13,HIGH);
        digitalWrite(relaypin,LOW);
        while(1){

        }
      }*/
      //retrieve new filename
      for (uint8_t i = 0; i < 100; i++) {
        filename[6] = '0' + i/10;
        filename[7] = '0' + i%10;
        // create if does not exist, do not open existing, write, sync after write
        if (!SD.exists(filename)) {
          break;
        }
      }
      logfile=SD.open(filename, FILE_WRITE);
      j-=100;
      }
    j++;

  }
}
