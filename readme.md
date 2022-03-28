## Alarm Clock

De fysiske delene av klokken består av: 3 knapper, 1 potentiometer, 2 LED, 1 piezo og en LCD. De tre knappene er koblet sammen i en resistorstige for å minimere kabling. 
En stor del av løsningen består i oppstykking i funksjoner. F.eks. bruker både alarm-velger funksjonen (alarmSet) og klokke-stille funksjonen (clockSet), de samme tre hjelpefunksjonene (hourSelect, minuteSelect og secondSelect). Disse tre funksjonene benytter seg videre av lcdTimeSet, som viser en angitt tid på skjermen.
Det er konfigurert en timer for å holde styr på tiden. Her er 256 blitt valgt som prescaler og 62500 satt til compare, ettersom 16Mhz/256 == 62500 == 1s.
Styring av LCD-en baserer seg på å oppdatere den minimalt. Slik unngås unødvendig flimring på displayet. Lcd.clear() funksjonen brukes nesten ikke, ettersom det er raskere å bare overskrive resten av linjen med mellomrom.

 - Klokken viser tid i timer, minutter og sekunder. Dette skjer gjennom oppdatering av tidsvariabler fra timer-ISR (Interrupt Service Routine), og videre visning av dem på skjermen gjennom funksjonen lcdTimeSet;
 - Brukeren kan stille inn klokken gjennom å trykke meny-knappen, så velge still klokke. Da brukes funksjonen clockSet.
 - Brukeren kan velge en alarm gjennom å trykke meny-knappen, så velge sett alarm. Denne lagres i EEPROM, og lastes ved oppstart. Håndtering av alarmvalg skjer gjennom funksjonen alarmSet. Det røde lyset skrus også på om en alarm er valgt.
 - Alarmen går av ved hjelp av Arduinos tone() funksjon når alarmtiden == nåtid.
 - Meny-knappen brukes som snooze knapp når alarmen ringer. Da skrus det grønne lyset på, alarmen av, og alarmen ringer igjen om 5min uten at selve alarmtiden endres. Om man trykker tilbake-knappen, skrus alarmen av og alarmen slettes, mens om man trykker velg-knappen skrus alarmen bare av, slik at den ringer til samme tid i morgen.

Videre er noen ekstra funksjoner implementert:
 - Man kan velge mellom 12- og 24-timers klokke. Om man velger 12-timers vises også AM/PM
 - Man kan angi snooze varighet fra 1 til 15 min. Default er 5 min.
Endring av ringelyd var planlagt, men ettersom jeg ikke er særlig musikalsk anlagt, ble denne ideen forkastet. Melodiene som ble utprøvd var av en slik kvalitet at det antakelig er mer komfortabelt å våkne til én repeterende tone, slik det er i koden per nå.

![klokken](hhtps://raw.githubusercontent.com/Kaexel/alarm_clock/images/alarm_menu.png)