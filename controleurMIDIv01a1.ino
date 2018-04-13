//###############################
//#  Load with 8Mhz ATmega 328  #
//###############################

bool debug = true;
#include <rotary.h>

//Rotary encoder var
Rotary rAnalyser = Rotary(2, 3);
volatile byte count = 0;


//Shift register var
#define data 4    // pin 4 for data
#define clock 5   // pin 5 for clock
byte dot=LOW;     // dotted number flag

//7-segment display multiplexing byte array
byte digit[10]=       {B00000011, B10011111, B00100101, B00001101, B10011001, B01001001, B01000001, B00011111, B00000001, B00001001}; 

//dotted 7-segment display multiplexing bytes array for notifing tenths
byte dottedDigit[10]= {B00000010, B10011110, B00100100, B00001100, B10011000, B01001000, B01000000, B00011110, B00000000, B00001000};

// busy animation sequence when switching between midi channel select and controller bank select
byte busyState[8]={B01111111,B10111111,B11111101,B11110111,B11101111,B11011111,B11111101,B11111011};

//All digits off
byte digitOff = B11111111;

//rotary encoder state flags
byte storeCount;
byte storeChannel=1;
bool storeDotA;
bool storeDotB;
long previousMillis = 0;

//MIDI var
//Selection canal (1-16);
volatile byte channel = 0;
byte bank = 0;
volatile bool MIDICHNSLCT=false;
volatile bool BANKSLCT=false;

//POT var
//Analog pin ZERO
int valuePinZero=0;
int valuePinZero2=0;
//Analog pin ONE
byte valuePinOne=0;
byte valuePinOne2=0;
//Analog pin TWO
byte valuePinTwo=0;
byte valuePinTwo2=0;
//Analog pin THREE
byte valuePinThree=0;
byte valuePinThree2=0;
//Analog pin FOUR
byte valuePinFour=0;
byte valuePinFour2=0;
//Analog pin FIVE
byte valuePinFive=0;
byte valuePinFive2=0;

void setup()
{
  Serial.begin(31250);  //MIDI:  31250
  pinMode(clock, OUTPUT); // Set clock pin
  pinMode(data , OUTPUT); // Set dada pin
  pinMode(6, INPUT_PULLUP);
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
  shiftOut(data, clock, LSBFIRST, digit[0]);
}

void loop()
{
  readAnalog();
  if(!digitalRead(6)&&!MIDICHNSLCT&&!BANKSLCT){
      for (int i=0; i <= 7; i++){
      shiftOut(data, clock, LSBFIRST, busyState[i]);
      delay(50);
      }
    storeCount=count;
    storeDotA=dot;
    count=storeChannel;
    dot=storeDotB;    
    refreshMIDICHNSLCT();
    MIDICHNSLCT=true;
    commandControl();
  }
}

void commandControl(){

  while(MIDICHNSLCT){
    
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis > 200){
      previousMillis = currentMillis;
      if(!debug){
        shiftOut(data, clock, LSBFIRST, digitOff);
        debug=true;
      }
      else if(!dot){
        shiftOut(data, clock, LSBFIRST, digit[count]);
        debug=false;
      }else{
        shiftOut(data, clock, LSBFIRST, dottedDigit[count]);
        debug=false;        
      }
    }
    if(!digitalRead(6)){
      for (int i=0; i <= 7; i++){
        shiftOut(data, clock, LSBFIRST, busyState[i]);
        delay(50);
      }
      storeDotB=dot;
      dot=storeDotA;
      storeChannel=count;
      count=storeCount;
      refresh();
      MIDICHNSLCT=false;;
    }
  }
}

void refresh(){
  cli();
    if(count>9&&count<255&&!dot){
      count=0;
      dot=HIGH;
    }
    if(count>2&&count!=255&&dot){
      dot=LOW;
      count=0;
    }
  if(count==255&&!dot){
      dot=HIGH;
      count=2;
    }
  if(count==255&&dot){
    dot=LOW;
    count=9;
  }
  
  if(!dot){
    shiftOut(data, clock, LSBFIRST, digit[count]);
    bank=count*6;
  }
  else{
    shiftOut(data, clock, LSBFIRST, dottedDigit[count]);
    bank=count*6+60;
  }
  
  sei();
}
void refreshMIDICHNSLCT(){
  cli();
    if(count>9&&!dot){
      count=0;
      dot=HIGH;
    }
    if(count>6&&count!=255&&dot){
      dot=LOW;
      count=1;
    }
  if(!count&&!dot){
      dot=HIGH;
      count=6;
    }
  if(count==255&&dot){
    dot=LOW;
    count=9;
  }
  if(!dot){
    shiftOut(data, clock, LSBFIRST, digit[count]);
    channel=count;
  }
  else{
    shiftOut(data, clock, LSBFIRST, dottedDigit[count]);
    channel=count+10;
  }
  sei();
}

//Interruption handler
ISR(PCINT2_vect) {
  unsigned char result = rAnalyser.process();
  if (result) {
    result == DIR_CW ? count-- : count++;
    if(!MIDICHNSLCT){
      refresh();
    }else{
      refreshMIDICHNSLCT();
      channel--;
    }
  }
}

//ADC read
void readAnalog(){
//controleur ZERO
//————————
valuePinZero = (analogRead(0)/4);
if (valuePinZero-valuePinZero2 >=2 || valuePinZero2-valuePinZero >=2) { 
valuePinZero2 = valuePinZero;
MIDI_TX(176+channel,0+bank, valuePinZero/2); //3 bytes MIDI message
}
 
//controleur ONE
//————————
valuePinOne = (analogRead(1)/4);
if (valuePinOne - valuePinOne2 >=2 || valuePinOne2 - valuePinOne >=2){
valuePinOne2 = valuePinOne;
MIDI_TX(176+channel,1+bank, valuePinOne/2);
//delay(100);
}
 
//controleur TWO
//————————
valuePinTwo = (analogRead(2)/4);
if (valuePinTwo - valuePinTwo2 >=2 || valuePinTwo2 - valuePinTwo >=2){
valuePinTwo2 = valuePinTwo;
MIDI_TX(176+channel,2+bank, valuePinTwo/2);
//delay(100);
}

//controleur THREE
//————————
valuePinThree = (analogRead(3)/4);
if (valuePinThree - valuePinThree2 >=2 || valuePinThree2 - valuePinThree >=2){
valuePinThree2 = valuePinThree;
MIDI_TX(176+channel,3+bank, valuePinThree/2);
//delay(100);
}
//controleur FOUR
//————————
valuePinFour = (analogRead(4)/4);
if (valuePinFour - valuePinFour2 > 2 || valuePinFour2 - valuePinFour >=2){
valuePinFour2 = valuePinFour;
MIDI_TX(176+channel,4+bank, valuePinFour/2);
//delay(100);
}
//controleur FIVE
//————————
valuePinFive = (analogRead(5)/4);
if (valuePinFive - valuePinFive2 > 2 || valuePinFive2 - valuePinFive >=2){ 
valuePinFive2 = valuePinFive;
MIDI_TX(176+channel,5+bank, valuePinFive/2);
//delay(100);
}
}

//Send midi over serial
void MIDI_TX(unsigned char MESSAGE, unsigned char DONNEE1, unsigned char DONNEE2){ //
  Serial.write(MESSAGE);
  Serial.write(DONNEE1);
  Serial.write(DONNEE2);
}


