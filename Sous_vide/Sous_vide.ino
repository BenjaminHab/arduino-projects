#include <SPI.h>
#include <SD.h>
#include <Adafruit_MAX31865.h>

// Set the pins used
Adafruit_MAX31865 max = Adafruit_MAX31865(9, 10, 12, 13);

//card pin
#define cardSelect 4
//refernence resistor value of the MAX31865 breakout board
#define RREF 430.0
//relaypin
#define relaypin 11
//if verbose==1, establish serial connection and print info to serial monitor
#define verbose 1
//type of used relay: 'SSR' or 'STD'. Mainly determines the PWM frequency (std PWM frequency vs. 1/T)
#define relaytype 'STD'

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
int T=5000;

//PWM variable
int dt=0;

unsigned long t=millis()+T;
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
  //establish serial connection, if verbose output is desired
  if(verbose==1){
    Serial.begin(115200);
    while(!Serial){};
    Serial.println("\r\n Begin setup...");
  }

  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect) && verbose==1) {
    Serial.println("Card init. failed!");
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
  //print progress or error to Serial
  if( ! logfile && verbose==1 ) {
    Serial.print("Couldnt create ");
    Serial.println(filename);
  }

  if(verbose==1){
    Serial.print("Writing to ");
    Serial.println(filename);
  }

  //set pinmodes for relay and 2nd LED
  pinMode(relaypin, OUTPUT);
  pinMode(8, OUTPUT);


  //initialize temperature sensor
  max.begin(MAX31865_2WIRE);
  if(verbose==1){
    Serial.println("Ready!");
  }

}


void loop() {
  //check, if timer has expired
  if(t<millis()){
    //reset timer
    t=millis()+T;
    //flash LED
    digitalWrite(8, HIGH);
    //read temperature and calculate control output
    y=max.temperature(100, RREF);
    u=PID(w-y,T);
    //log time, PID out and temperature
    logfile.print(millis()); logfile.print(", "); logfile.print(u); logfile.print(", "); logfile.println(y);
    if(verbose==1){
      Serial.print(millis()); Serial.print(", "); Serial.print(u); Serial.print(", "); Serial.println(y);
    }
    //switch off the LED
    digitalWrite(8, LOW);

    //write changes to card
    if(j>=100){
      logfile.flush();
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

    //write u to the relay
    switch(relaytype){
      //SSR-Version
      case 'SSR':
        analogWrite(relaypin,u);
        break;

      case 'STD':
        //normal Relay low frequency PWM version
        if(u){
          //pulse width in milliseconds
          dt=int(u*t/255);
          //write pulse to relay
          digitalWrite(relaypin,HIGH);
          delay(dt);
          digitalWrite(relaypin,LOW);
        }
        break;

      default:
        if(verbose==1){
          Serial.println("incorrect input for variable relaytype");
        }
        break;
    }

  }
}
