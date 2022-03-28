#include <Arduino.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

#include <alarmStruct.hpp>
#include <helper.hpp>

#define LEDGREEN (1 << 3)
#define LEDRED (1 << 4)

#define PIN_LEDS (LEDGREEN | LEDRED)

int manualAnalogRead(int pin);
void mainMenu();
void clockSet();
void alarmSet();
void settings();

int hourSelect(int hours, int minutes, int seconds);
int minuteSelect(int hours, int minutes, int seconds);
int secondSelect(int hours, int minutes, int seconds);

int snoozeLengthSet();
void timeModeSet();
void ringerSet();
void alarmHandler(bool alarm);

void lcdTimeSet(int hour, int min, int sec);
void lcdMainMenuSet(int potVal);
void lcdSettingsMenuSet(int potVal);

volatile int secondsCurrent = 0;
volatile int minutesCurrent = 0;
volatile int hoursCurrent = 12;
alarm_t alarmStruct;
int eepromAddress = 0;

LiquidCrystal lcd = LiquidCrystal(8,9,3,4,5,6);


String mainMenuChoices[] {
  "Still klokka",
  "Sett alarm",
  "Innstillinger"
};

String settingsChoices[] {
  "Snooze-lengde",
  "Tidsmodus",  
  "Ringelyd"    //TODO: implementere ringelydvalg
};

int timeMode = 24;             //brukes for å bytte mellom 12- og 24-timers klokke
bool pm = true;                //når alarmen skrus på er det 12 midt på dagen, dvs. 12PM

bool is_alarm_set = false;     //
bool alarm_ringing = false;    //er true når alarm ringer
bool snooze = false;           //settes true når snooze trykkes på
int snoozeTimer;               //brukes for å vite hvor lenge det er snoozet
volatile int snoozeLength = 5; //lengde på snooze(i minutt)

bool printTime = true; //denne variabelen passer på at interruptvektoren ikke prøver å skrive tiden

void setup() {  

 //konfigurerer en timer
  cli();

    //resetter kontrollregistre
    TCCR1A = 0;
    TCCR1B = 0;

    //skrur på mode 4 (CTC-mode)
    TCCR1B |= (1 << WGM12);

    //velger prescaler (256)
    TCCR1B |= (1 << CS12);

    //setter compare til 62500 ((16MHz / prescaler) == 1s == 62500)
    OCR1A = 62500;

    //skrur på Output Compare Match A Interrupt Vector
    TIMSK1 |= (1 << OCIE1A);

  sei();

  
  //grønn led er pin 10, rød led er pin 11, piezo er pin 2

  //setter LEDer til output
  DDRB |= PIN_LEDS;

  //setter opp ADC
  ADCSRA |= (1 << 7);
  ADMUX &= ~(0xF);
  ADMUX |= (1 << REFS0);

  //henter lagret alarmstruct fra eeprom
  EEPROM.get(eepromAddress, alarmStruct);

  if(alarmStruct.seconds >= 0 && alarmStruct.seconds <= 59) {
    PORTB |= LEDRED;
  } else {
    PORTB &= ~LEDRED; 
  }

  Serial.println("---------");
  Serial.println(alarmStruct.hour);
  Serial.println(alarmStruct.minutes);
  Serial.println(alarmStruct.seconds);
  Serial.println("---------");

  lcd.begin(16, 2);

  Serial.begin(9600);
}


void loop() {

  int buttonVal = manualAnalogRead(0);
  printTime = true;

  alarmHandler(alarm_ringing);
  
  //det brukes en resistorstige for knappene
  if (buttonVal >= 1020 && buttonVal <= 1023) { //menyknapp
    printTime = false;
    mainMenu();
  } 
}

//funksjon for å lese verdi for en gitt analogpin
int manualAnalogRead(int pin) {

  ADMUX &= ~(0xF);
  ADMUX |= (pin);

  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));

  int val = ADCL;
  val |= (ADCH << 8);
  return val;
}

//funksjon for å vise hovedmeny
void mainMenu() {

  int oldPot = 0;
  int potVal = 0;
  lcdMainMenuSet(potVal);
  while(1) {
    int buttonVal = manualAnalogRead(0);
    int potVal = manualAnalogRead(1);
    int diff = difference(potVal, oldPot);

    if (diff > 20) { //passer på å ikke opppdatere LCD for ofte
      oldPot = potVal;
      lcdMainMenuSet(potVal);
    }

    if((buttonVal > 800 && buttonVal < 1010) || alarm_ringing)  {
      break;
    } 
    if (buttonVal > 400 && buttonVal < 600) {
      if(potVal < 500) {
        clockSet();
        lcdMainMenuSet(potVal);
      } else if (potVal > 500 && potVal < 1000) {
        alarmSet();
        lcdMainMenuSet(potVal);
      } else {
        settings();
      }
    }
    
    delay(10);
  }
}

//funksjon for å vise innstillingsmeny
void settings() {
  int oldPot = 0;
  int potVal = 0;
  lcdSettingsMenuSet(potVal);
  while(1) {
    int buttonVal = manualAnalogRead(0);
    int potVal = manualAnalogRead(1);
    int diff = difference(potVal, oldPot);

    if (diff > 10) { //passer på å ikke opppdatere LCD for ofte
      oldPot = potVal;
      lcdSettingsMenuSet(potVal);
    }

    if((buttonVal > 800 && buttonVal < 1010) || alarm_ringing)  { //viser klokka om man går tilbake eller alarmen ringer
      break;
    } 
    if (buttonVal > 400 && buttonVal < 600) {
      if(potVal < 500) {
        snoozeLengthSet();
        lcdSettingsMenuSet(potVal);
        delay(200);
      } else if (potVal > 500 && potVal < 1000) {
        timeModeSet();
        lcdSettingsMenuSet(potVal);
        delay(200);
      } else {
        ringerSet();
        lcdSettingsMenuSet(potVal);
        delay(200);
      }
    }
    delay(10);
  }
}

//funksjon for å håndtere ringende alarm
void alarmHandler(bool alarm) {
  
  if (alarm) { 
    int buttonVal = manualAnalogRead(0);
    tone(2, 561, 50);
     //hvis alarm ringer og menyknappen trykkes, skal alarmen snoozes (dette håndteres av timeren)
    if (buttonVal >= 1020 && buttonVal <= 1023) {
      snooze = true;
      PORTB |= LEDGREEN;
      alarm_ringing = false;
    }
      //hvis alarm ringer og tilbakeknappen trykkes, skal alarmen skrus av og fjernes
    if (buttonVal > 800 && buttonVal < 1010) {
      alarm_ringing = false;

      alarmStruct.hour = 0;
      alarmStruct.minutes = 0;
      alarmStruct.seconds = 70; //her settes alarm-structen til en verdi som ikke kan oppnås for å skru av alarmen helt
      EEPROM.put(eepromAddress, alarmStruct);
      PORTB &= ~LEDRED;
    }
      //hvis alarm ringer og velg-knappen trykkes skrus bare alarmen av
    if(buttonVal > 400 && buttonVal < 600) {
      alarm_ringing = false; //her beholder alarmen sin forrige verdi, slik at den vil ringe på samme tid imorgen
    }

  }
}

//funksjon for å velge alarm 
void alarmSet() {
  delay(500);
  int alarmSetHour = hourSelect(hoursCurrent, minutesCurrent, secondsCurrent);
  delay(500); 
  int alarmSetMinute = minuteSelect(alarmSetHour, minutesCurrent, secondsCurrent);
  delay(500);
  int alarmSetSeconds = secondSelect(alarmSetHour, alarmSetMinute, secondsCurrent);

  alarmStruct.hour = alarmSetHour;
  alarmStruct.minutes = alarmSetMinute;
  alarmStruct.seconds = alarmSetSeconds;

  EEPROM.put(eepromAddress, alarmStruct); //legger alarm i EEPROM
  is_alarm_set = true;
  PORTB |= LEDRED; //skrur på rød led


  Serial.println(alarmStruct.hour);
  Serial.println(alarmStruct.minutes);
  Serial.println(alarmStruct.seconds);

  delay(500);
  
  lcdTimeSet(alarmSetHour, alarmSetMinute, alarmSetSeconds);
  lcd.setCursor(0,1);
  lcd.print(String("Alarm satt"));
  delay(1000);
  return;
}

//funksjon for å stille klokke
void clockSet() {

  delay(500);
  int clockSetHour = hourSelect(hoursCurrent, minutesCurrent, secondsCurrent);
  delay(500); 
  int clockSetMinute = minuteSelect(clockSetHour, minutesCurrent, secondsCurrent);
  delay(500);
  int clockSetSeconds = secondSelect(clockSetHour, clockSetMinute, secondsCurrent);

  hoursCurrent = clockSetHour;
  minutesCurrent = clockSetMinute;
  secondsCurrent = clockSetSeconds;

  delay(500);
  
  lcdTimeSet(hoursCurrent, minutesCurrent, secondsCurrent);
  lcd.setCursor(0,1);
  lcd.print(String("Klokke stilt"));
  delay(1000);
  return;
  

}

//funksjon for å velge time
int hourSelect(int hours, int minutes, int seconds) {

   int potVal = 0;
   int oldPot = 0;

   lcdTimeSet(hours, minutes, seconds);
   lcd.setCursor(0,1);
   lcd.print("Velg time");

  while(1) {
    int buttonVal = manualAnalogRead(0);
    potVal = manualAnalogRead(1);

    int hour = map(potVal, 0, 1020, 0, 23);
    int diff = difference(oldPot, potVal);
    
    if(diff > 10) {
      oldPot = potVal;
      lcdTimeSet(hour, minutes, seconds);
      lcd.setCursor(0,1);
      lcd.print("Velg time");
    }
  
    delay(10);

    if(buttonVal > 400 && buttonVal < 600) {
      return hour;
    }
  }
}

//funksjon for å velge minutt
int minuteSelect(int hours, int minutes, int seconds) {
  
  int potVal = 0;
  int oldPot = 0;

  lcdTimeSet(hours, minutes, seconds);
  lcd.setCursor(0,1);
  lcd.print("Velg minutt");

  while(1) {
    int buttonVal = manualAnalogRead(0);
    potVal = manualAnalogRead(1);

    int minute = map(potVal, 0, 1020, 0, 59);
    int diff = difference(oldPot, potVal);
    if (diff > 10) {
      oldPot = potVal;
      lcdTimeSet(hours, minute, seconds);
      lcd.setCursor(0,1);
      lcd.print("Velg minutt");
    }
  
    delay(10);

    if(buttonVal > 400 && buttonVal < 600) {
      return minute;
    }
  }

}

//funksjon for å velge sekund
int secondSelect(int hours, int minutes, int seconds) {

  int potVal = 0;
  int oldPot = 0;

  lcdTimeSet(hours, minutes, seconds);
  lcd.setCursor(0,1);
  lcd.print("Velg sekund");

  while(1) {
    int buttonVal = manualAnalogRead(0);
    potVal = manualAnalogRead(1);

    int second = map(potVal, 0, 1020, 0, 59);
    int diff = difference(oldPot, potVal);
    if(diff > 10) {
      oldPot = potVal;
      lcdTimeSet(hours, minutes, second);
      lcd.setCursor(0,1);
      lcd.print("Velg sekund");
    }

    delay(10);

    if(buttonVal > 400 && buttonVal < 600) {
      return second;
    }
  }

}

//denne funksjonen setter lengde på snooze fra 1 til 15 min
int snoozeLengthSet() { 

  int potVal = 0;
  int oldPot = 0;
  lcdTimeSet(0, 0, 0);
  delay(100);

  while(1) {
    int buttonVal = manualAnalogRead(0);
    potVal = manualAnalogRead(1);

    int diff = difference(oldPot, potVal);

    int snoozeLengthAdjust = map(potVal, 0, 1020, 1, 15);
    

    if (diff > 10) {
      oldPot = potVal;
      lcdTimeSet(0, snoozeLengthAdjust, 0);
      lcd.setCursor(0,1);
      lcd.print("Snooze (i min)      ");
    }

    delay(10);

    if(buttonVal > 400 && buttonVal < 600) {
      snoozeLength = snoozeLengthAdjust;
      return 1;

    }
  }

}

//denne funksjonen velger mellom 12- og 24-timers klokke
void timeModeSet() {
  int potVal = 0;
  int oldPot = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("12-timers");
  lcd.setCursor(0,1);
  lcd.print("24-timers");
  delay(100);

  while(1) {
    int buttonVal = manualAnalogRead(0);
    potVal = manualAnalogRead(1);

    int diff = difference(oldPot, potVal);

    if (diff > 300) {
      if(potVal > 500) {
        lcd.setCursor(0,0);
        lcd.print("12-timers<-     ");
        lcd.setCursor(0,1);
        lcd.print("24-timers     ");
      } else {
        lcd.setCursor(0,0);
        lcd.print("12-timers     ");
        lcd.setCursor(0,1);
        lcd.print("24-timers<-     ");
      }
    }

    if(buttonVal > 400 && buttonVal < 800) {
      if(potVal > 500) {
        timeMode = 13;
        return;
      }
      else {
        timeMode = 24;
        return;
      }
    }
 

    if(buttonVal > 800 && buttonVal < 1010) {
      return;
    }
    delay(10);
  }

}

//funksjon for å velge ringelyd
void ringerSet() {
    //TODO: implemetere dette
}

//styrer lcd for hovedmeny
void lcdMainMenuSet(int potVal) {

    
    lcd.setCursor(0,0);
    
    if (potVal < 500) {
      lcd.print(mainMenuChoices[0] + "<-       ");
    } else if (potVal > 500 && potVal < 1000) {
      lcd.print(mainMenuChoices[1] + "<-     ");
    } else {
      lcd.print(mainMenuChoices[2] + "<-     ");
    }
    delay(10);
    
    lcd.setCursor(0, 1);  
    if (potVal < 500) {
      lcd.print(mainMenuChoices[1] + "      ");
    } else if (potVal > 500 && potVal < 1000) {
      lcd.print(mainMenuChoices[2] + "       ");
    } else {
      lcd.print("                ");
    }
    delay(10);
}

//styrer lcd for settingsmeny
void lcdSettingsMenuSet(int potVal) {
 
    lcd.setCursor(0,0);
    if (potVal < 500) {
      lcd.print(settingsChoices[0] + "<-      ");
    } else if (potVal > 500 && potVal < 1000) {
      lcd.print(settingsChoices[1] + "<-      ");
    } else {
      lcd.print(settingsChoices[2] + "<-      ");
    } 
    delay(10);
    
    lcd.setCursor(0, 1);  
    if (potVal < 500) {
      lcd.print(settingsChoices[1] + "        ");
    } else if (potVal > 500 && potVal < 1000) {
      lcd.print(settingsChoices[2] + "        ");
    } else {
      lcd.print("         ");
    }
    delay(10);
}

//viser tiden på skjermen
void lcdTimeSet(int hour, int min, int sec) {
  String secString;
  String minString;
  String hourString;

  if(timeMode == 13) { //justering for 12-timers klokke
    if(hour >= 12) {
      if (hour > 12) {
        hour -= 12;
      }
      pm = true;
    } else if(hour == 0) {
      hour = 12;
      pm = false;
    } else {
      pm = false;
    }
  }

  //koden under legger på et null om tiden er ensifret  
  if (sec < 10) {
    secString = 0 + String(sec);
  } else {
    secString = String(sec);
  }

  if (min < 10) {
    minString = 0 + String(min);
  } else {
    minString = String(min);
  }

  if (hour < 10) {
    hourString = 0 + String(hour);
  } else {
    hourString = String(hour);
  }



  lcd.setCursor(0,0);
  lcd.print(hourString + String(':') + minString + String (':') + secString + String("        "));
  lcd.setCursor(0,1);
  lcd.print("               ");
  
  if(timeMode == 13) {
    lcd.setCursor(14, 0);
    if (pm) {
      lcd.print("PM");
    } else {
      lcd.print("AM");
    }
  }
}

//interrupt service for timeren
ISR(TIMER1_COMPA_vect) {

 //denne kodesnutten håndterer snoozefunksjonalitet
 if (snooze) {
    snoozeTimer += 1;
    if (snoozeTimer >= (snoozeLength * 60)) { //sjekker på snoozetimer, default == 5min
      snooze = false;
      PORTB &= ~LEDGREEN;
      alarm_ringing = true;
    }
  } else {
    snoozeTimer = 0;
  }

 secondsCurrent++;

  if (secondsCurrent >= 60) {
    minutesCurrent += 1;
    secondsCurrent = 0;
  }
  if (minutesCurrent >= 60) {
    hoursCurrent += 1;
    minutesCurrent = 0;
  }
  if(hoursCurrent >= 24) {
    hoursCurrent = 0;
  }
  Serial.println(String(alarmStruct.hour) + String(':') + String(alarmStruct.minutes) + String(':') + String(alarmStruct.seconds));
  Serial.print(snoozeTimer);

  if (hoursCurrent == alarmStruct.hour && minutesCurrent == alarmStruct.minutes && secondsCurrent == alarmStruct.seconds) {
    alarm_ringing = true;
  }

  if(printTime) {
    lcdTimeSet(hoursCurrent, minutesCurrent, secondsCurrent);
  } 

}

