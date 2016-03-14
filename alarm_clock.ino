#include <Wire.h>
#include <RTClib.h>
#include <SoftwareSerial.h>

//Pin variables for the various functions
int hourPin=2,minutePin=3,secondPin=4,alarmActivePin=6,alarmEditPin=5,alarmbuttonPin=8,pmLightPin=9,alarmActiveLightPin=11,alarmEditLightPin=10;

//setup pins for s7s display
const int softwareTx = 7;
const int softwareRx = 12;

RTC_DS1307 RTC;
SoftwareSerial s7s(softwareRx, softwareTx);

char tempString[10]; 
bool AMtime,editAlarm,activeAlarm,alarmRinging;
int hour12,alarmhour12,alarmHour,alarmMinute,cooldown;

void setup () {
    //initializing values
    AMtime=false;editAlarm=false;activeAlarm=false;alarmRinging=false;
    alarmHour=12;alarmMinute=12;cooldown=-1;

    
    //Serial.begin(57600); *For testing through the console*
    Wire.begin();
    RTC.begin();

    //Start s7s software serial at right baud rate
    s7s.begin(9600);
    clearDisplay();
    //print test message to ensure correctly functioning display
    s7s.print("TEST");
    //set brightness of display to max
    setBrightness(255);
    delay(3000);
    clearDisplay();  

  pinMode(hourPin, INPUT); //hour adjustment input
  pinMode(minutePin, INPUT); //minute adjustment input
  pinMode(secondPin, INPUT); //zero out seconds input
  pinMode(alarmEditPin,INPUT);//Edit alarm time
  pinMode(alarmActivePin,INPUT);//Set alarm as active
  pinMode(alarmbuttonPin,INPUT);//Turn off alarm
  
  pinMode(pmLightPin, OUTPUT);//AM/PM LED Indicator
  pinMode(alarmActiveLightPin, OUTPUT);//Active alarm indicator
  pinMode(alarmEditLightPin, OUTPUT);//Edit alarm indicator

}

void loop () {

 DateTime now = RTC.now(); //get current time from the 1307 RTC

//Check if alarm is active
if(digitalRead(alarmActivePin)==HIGH){
  activeAlarm=true;
  digitalWrite(alarmActiveLightPin,HIGH);
}else{
  activeAlarm=false;
  digitalWrite(alarmActiveLightPin,LOW);
}

//Check if edit mode is selected
if(digitalRead(alarmEditPin)==HIGH){
  editAlarm=true;
  digitalWrite(alarmActiveLightPin,LOW);
  digitalWrite(alarmEditLightPin,HIGH);
}else {
  digitalWrite(alarmEditLightPin,LOW);
  editAlarm=false;
}

//Convert and format 24 hour time to 12 hour time (ALARM)
//Remove this code to keep the output as 24 hour
if(alarmHour>=12 && editAlarm==true){
  AMtime=false;
  alarmhour12 = alarmHour-12;
  if(alarmHour==12)alarmhour12 = alarmHour;
}else {
  AMtime=true;
  alarmhour12 = alarmHour;
  if(alarmHour==0)alarmhour12 = 12;
}

//Convert and format 24 hour time to 12 hour time (CLOCK)
//Remove this code to keep the output as 24 hour
if(now.hour()>=12 && editAlarm==false){
  AMtime=false;
  hour12 = now.hour()-12;
  if(now.hour()==12)hour12 = now.hour();
}else if(editAlarm==false){
  AMtime=true;
  hour12 = now.hour();
  if(now.hour()==0)hour12 = 12;
}
  
  // create string to send to the s7s
  //To update display in 24 hour time replace hour12,alarmhour12 with now.hour() and alarmHour
  if(editAlarm!=true && alarmRinging!=true){
    if(now.minute()>=10)sprintf(tempString, "%2d%2d", hour12,now.minute());
    else sprintf(tempString, "%2dO%1d", hour12,now.minute());
  }
  if(editAlarm==true && alarmRinging!=true){
    if(alarmMinute>=10)sprintf(tempString, "%2d%2d",alarmhour12,alarmMinute);
    else sprintf(tempString, "%2dO%1d",alarmhour12,alarmMinute);
  }

//check if cooldown for alarm has expired
if(now.minute()==cooldown)cooldown=-1;
  
//Check if alarm is ringing
if(now.hour()==alarmHour && now.minute()==alarmMinute && editAlarm==false && cooldown==-1 && activeAlarm==true)alarmRinging=true;

//Do following when alarm is activated (Example just prints RING to s7s)
if(alarmRinging==true) sprintf(tempString, "RING");

//Turn off alarm ringing
  if(alarmRinging==true){
    if(digitalRead(alarmbuttonPin)==HIGH){
      alarmRinging=false;
      sprintf(tempString, "DONE");
      //Activate cooldown timer, prevents alarm from ringing until timer expires 3 minutes after alarm activation time
      if(now.minute()<56)cooldown=now.minute()+3;
      else cooldown = 0;
    }
  }

//Set state of AM/PM light pin
  if(AMtime==false)digitalWrite(pmLightPin, HIGH);
  else digitalWrite(pmLightPin, LOW);

  // Output the tempString to the s7s
  s7s.print(tempString);
  setDecimals(0b00010000);  //set the colon lights to active

  delay(100);  // This will make the display update at 10Hz.

/*
 * Some debugging test prints to Serial
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

//delay(3000);
*/

//Alarm Settings

  //add one hour when pressed
  if(digitalRead(hourPin) == HIGH && editAlarm==true) {
    //Serial.println("CHANGED HOUR + 1");
    if(alarmHour == 23) {
       alarmHour=0; //reset the hour back to zero
    } else {
    alarmHour+=1;
    }
    delay(500);//switch debounce i.e wait 500ms before applying logic again (prevents spam of command)
  }
  // add one minute when pressed
  if(digitalRead(minutePin) == HIGH && editAlarm==true) {
    //Serial.println("CHANGED MINUTE + 1");
    if(alarmMinute == 59) {//if minutes equal 59, set to zero and increment hour +1
       alarmMinute=0;
       if(alarmHour<23)alarmHour+=1;//make sure to set hour to zero if equal to 23 and minute rolls over
       else alarmHour=0;
    } else {
   alarmMinute+=1;
    }
    delay(500);//switch debounce i.e wait 500ms before applying logic again (prevents spam of command)
  }

//Clock time settings

  //add one hour when pressed
  if(digitalRead(hourPin) == HIGH && editAlarm==false) {
    //Serial.println("CHANGED HOUR + 1");
    if(now.hour() == 23) {
       RTC.adjust(DateTime(now.year(), now.month(), now.day(), 0, now.minute() , 0)); //reset the hour back to zero
    } else {
    RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() + 1, now.minute(), now.second()));
    }
    delay(500);//switch debounce i.e wait 500ms before applying logic again (prevents spam of command)
  }
  // add one minute when pressed
  if(digitalRead(minutePin) == HIGH && editAlarm==false) {
    //Serial.println("CHANGED MINUTE + 1");
    if(now.minute() == 59) {
       if(now.hour()<23)RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour()+1, 0, 0));//if minutes equal 59, set to zero and increment hour +1
       else RTC.adjust(DateTime(now.year(), now.month(), now.day(), 0, 0, 0));//make sure to set hour to zero if equal to 23 and minute rolls over
    } else {
    RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute() + 1, now.second()));
    }
    delay(500);//switch debounce i.e wait 500ms before applying logic again (prevents spam of command)
  }
  
  //reset seconds
  if(digitalRead(secondPin) == HIGH) {
    //Serial.println("SECOND RESET");
    RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), 0));
    delay(250);//switch debounce i.e wait 250ms before applying logic again (prevents spam of command)
  }

}

// Send the clear display command (0x76)
// This will clear the display and reset the cursor
void clearDisplay()
{
  s7s.write(0x76);  // Clear display command
}

void setBrightness(byte value)
{
  s7s.write(0x7A);  // Set brightness command byte
  s7s.write(value);  // brightness data byte
}
//Activate specific decimals located on the s7s
void setDecimals(byte decimals)
{
  s7s.write(0x77);
  s7s.write(decimals);
}
