#include <Arduino.h>

// FTP library example
// by Industrial Shields

#include <FTP.h>
#include <SdFat.h>
#include <Ethernet.h>
#include <TimeLib.h>     // for update/display of time
#include <NTPClient.h>
#include <Adafruit_MAX31865.h>
//////////////////////////PT100 parameters
#define RREF      430.0
#define RNOMINAL  100.0
Adafruit_MAX31865 thermo0 = Adafruit_MAX31865(2,6,7,8);
Adafruit_MAX31865 thermo1 = Adafruit_MAX31865(3,6,7,8);
Adafruit_MAX31865 thermo2 = Adafruit_MAX31865(4,6,7,8);
Adafruit_MAX31865 thermo3 = Adafruit_MAX31865(5,6,7,8);

////////////////Ethernet MAC for DHCP
uint8_t mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x7B, 0x79 };  

////Global variables for Server
IPAddress server( 192, 168, 0, 193 );

const char *user = "arduino";
const char *pass = "12345";
char fileName [31] = "";
char mainDir [20] = "Temperature";
char yearDir [6] = "2022";
char monthDir[5] = "04";
char dayDir [5]= "20";

EthernetClient ftpControl;
EthernetClient ftpData;
FTP ftp(ftpControl, ftpData);


//////SD GLOBAL VARIABLES
const String logFileversion = "LogFile Version 2.1";
SdFat SD;
const long minfreesize =  3831670; ///free memory size in kB

////Global varibale for time library
EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP);
unsigned long temperatureTime = 0;
const int temperatureInterval = 1000; 

////////////////////////////////////////////////
#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

// Connect via i2c, default address #0 (A0-A2 not jumpered)
Adafruit_LiquidCrystal lcd(0);

////Other Global variables
const int numsensors = 4;
unsigned long timechange = 0; //timmer record for check time ftp
char min[5]="00";
char hours[5]="00";
bool reset =  true;
bool datachange = false;
unsigned long long Globaltime=0;
unsigned long long lastmillis=0;
float lastsecond = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
bool createSDfile(char*filename,String version){
  if(!SD.exists(filename)){
    File fh = SD.open(filename, FILE_WRITE);
    fh.println(version);
    fh.close();
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool writeSD( char*filename, String c){
  if(SD.exists(filename)){
    File fh = SD.open(filename, FILE_WRITE);
    if (fh) {
      fh.println(c);
      fh.close();
      return true;
    }else{
      Serial.print("Error al escribir en la tarjeta");
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
String getmilisecondsformat(float milis){
  String milisecondsstring = String(milis);
  milisecondsstring = milisecondsstring.substring((milisecondsstring.length()-2));
  if(milisecondsstring.length() == 3){
  }else{
    if(milisecondsstring.length() == 2){
      String c ="";
      c.concat(milisecondsstring);
      c.concat("0");
      milisecondsstring = c;
    }else{
      if(milisecondsstring.length() == 1){
        String c ="00";
        c.concat(milisecondsstring);
        milisecondsstring = c;
      }
    }
  }
  return milisecondsstring;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
String updatedsecondtimereference(){
  unsigned long nowmilis=millis();
  unsigned long elapsedtime=nowmilis-lastmillis;
  lastsecond = lastsecond +(float(elapsedtime)/1000);
  Serial.println(lastsecond);
  String dif=getmilisecondsformat(lastsecond);
  Serial.println(dif);
  if(lastsecond>1){
    Globaltime=Globaltime + int(lastsecond);
    lastsecond = lastsecond-int(lastsecond);
  }
  lastmillis=lastmillis+long(elapsedtime);
  return dif;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void delayfunction(int timedelay){
     unsigned start = millis();
     while(millis() < start+timedelay){
    // espere [periodo] milisegundos
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void restartDirectory(){
  if(!ftp.makedir(mainDir)){
    Serial.println(F("Error creating main directory"));
  }else{
    if(!ftp.changedir(mainDir)){
      Serial.println(F("Error changing main directory"));
    }else{
      if(!ftp.makedir(yearDir)){
        Serial.println(F("Error creating year directory"));
      }else{
        if(!ftp.changedir(yearDir)){
          Serial.println(F("Error changing year directory"));
        }else{
          if(!ftp.makedir(monthDir)){
            Serial.println(F("Error creating month directory"));
          }else{
            if(!ftp.changedir(monthDir)){
              Serial.println(F("Error changing month directory"));
            }else{
              if(!ftp.makedir(dayDir)){
                Serial.println(F("Error creating day directory"));
              }else{
                if(!ftp.changedir(dayDir)){
                  Serial.println(F("Error changing day directory"));
                }
              }
            }
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool updatedirectoryYear(){
  if(!ftp.changedir(yearDir)){
    Serial.println(F("Error changing year directory first time"));
    ftp.stop();
    while(!ftp.connect(server, user, pass)){
      Serial.println(F("Error connecting to FTP server"));
      delayfunction(100);
    }
    if(!ftp.changedir(mainDir)){
      return false;
    }else{
      if(!ftp.makedir(yearDir)){
        Serial.println(F("Error creating year directory"));
        return false;
      }else{
        ftp.changedir(yearDir);
        return true;
      }
    }
  }else{
    return true;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool updatedirectoryMonth(){
  if(!ftp.changedir(monthDir)){
    Serial.println(F("Error changing month directory first time"));
    ftp.stop();
    while(!ftp.connect(server, user, pass)){
      Serial.println(F("Error connecting to FTP server"));
      delayfunction(100);
    }
    if(!ftp.changedir(mainDir)){
      return false;
    }else{
      if(!updatedirectoryYear()){
        return false;
      }else{
        if(!ftp.makedir(monthDir)){
          Serial.println(F("Error creating month directory"));
          return false;
        }else{
          ftp.changedir(monthDir);
          return true;
        }
      }
    }
  }else{
    return true;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool updatedirectoryDay(){
  if(!ftp.changedir(dayDir)){
    Serial.println(F("Error changing day directory first time"));
    ftp.stop();
    while(!ftp.connect(server, user, pass)){
      Serial.println(F("Error connecting to FTP server"));
      delayfunction(100);
    }
    if(!ftp.changedir(mainDir)){
      return false;
    }else{
      if(!updatedirectoryYear()){
        return false;
      }else{
        if(!updatedirectoryMonth()){
          return false;
        }else{
          if(!ftp.makedir(dayDir)){
            Serial.println(F("Error creating day directory"));
            return false;
          }else{
            ftp.changedir(dayDir);
            return true;
          }
        }
      }
    }
  }else{
    return true;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void updateDirectory(){
  if(!ftp.changedir(mainDir)){
    Serial.println("Error changing main directory");
  }else{
    while(!updatedirectoryYear());
    delayfunction(10);
    while(!updatedirectoryMonth());
    delayfunction(10);
    while(!updatedirectoryDay());
    delayfunction(10);
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
long ShowFreeSpace() {
  // Calcular el espacio libre (volumen libre de los clusters * bloques por clústeres / 2)
  long lFreeKB = SD.vol()->freeClusterCount();
  lFreeKB *= SD.vol()->sectorsPerCluster()/2;

  // Mostrar el espacio libre
  Serial.print(F("Free space: "));
  Serial.print(lFreeKB);
  Serial.println(F(" KB"));
  return lFreeKB;
}
void setLCD(float sensor,int num){
  String c ="IP: ";
  IPAddress my = Ethernet.localIP();
  char myip[16];
  sprintf(myip, "%d.%d.%d.%d", my[0], my[1], my[2], my[3]);
  c.concat(String(myip));
  lcd.setCursor(0,0);
  lcd.print(c);
  unsigned long t = Globaltime;
  char time[14];
  sprintf(time, "%02d:%02d:%02d", hour(t), minute(t), second(t));
  c = "Time: ";
  c.concat(String(time));
  lcd.setCursor(0,1);
  lcd.print(c);
  c ="Sensor ";
  c.concat(num);
  c.concat(": ");
  c.concat(sensor);
  lcd.setCursor(0,3);
  lcd.print(c);

}
////////////////////////////////////////////////////////////////////////////////////////////////////
String setmessage(float sensor,int num){
  String c ="";
  String milisecondframe=updatedsecondtimereference();////////////////////
  unsigned long t = Globaltime;
  char time[29];
  sprintf(time, "%02d_%02d_%02d_%02d%02d%02d.", year(t), month(t), day(t), hour(t), minute(t), second(t));
  char timebuffer[5];
  sprintf(timebuffer, "%02d",minute(t));////////////////change it
  if(strcmp(timebuffer,min) !=0){
    memcpy(min,timebuffer,sizeof(timebuffer));
    datachange = true;
  }else{
    setLCD(sensor, num);
    c.concat(String(time));
    c.concat(milisecondframe);
    c.concat(";Sensor_");
    c.concat(String(num));
    c.concat("_mazzine;numbers;temperature;");
    c.concat(String(sensor));
    c.concat(";celsius");
  }
  return c;

}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool setSDframe(float temp,int i){
  bool successetSDframe=true;
  String c = setmessage(temp,i+1);
  if(!writeSD(fileName,c)){
    successetSDframe=false;
  }
  return successetSDframe;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void deleteOldestFile(){ 
  SdFile dirFile;
  SdFile file;
  SdFile oldestFile;
  dir_t dir;
  uint32_t oldestModified = 0xFFFFFFFF;
  uint32_t lastModified;
  if (!dirFile.open("/", O_READ)) {
    SD.errorHalt("open root failed");
  }
 
  file.openNext(&dirFile, O_WRITE);
  while (file.openNext(&dirFile, O_WRITE)) {
    // Skip directories and hidden files.
    if (!file.isSubDir() && !file.isHidden()) {
      file.dirEntry(&dir);
      lastModified = (uint32_t (dir.lastWriteDate) << 16 | dir.lastWriteTime); ///test uint32
      if (lastModified < oldestModified ) {
        oldestModified = lastModified;
        oldestFile = file;
      }
    }
    file.close();
  }
  oldestFile.remove();
  dirFile.close();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float readtemperature(int num,int numsamples){
  float temperature;
  switch (num){
    case 0:
      temperature = thermo0.temperature(RNOMINAL, RREF);
      for(int i = 0; i< numsamples; i++){
        temperature =temperature + (thermo0.temperature(RNOMINAL, RREF));
      }
      temperature = temperature/(numsamples+1);
      break;

    case 1:
      temperature =thermo1.temperature(RNOMINAL, RREF);
      for(int i = 0; i< numsamples; i++){
        temperature =temperature + (thermo1.temperature(RNOMINAL, RREF));
      }
      temperature = temperature/(numsamples+1);
      break;

    case 2:
      temperature = thermo2.temperature(RNOMINAL, RREF);
      for(int i = 0; i< numsamples; i++){
        temperature =temperature + (thermo2.temperature(RNOMINAL, RREF));
      }
      temperature = temperature/(numsamples+1);
      break;

    case 3:
      temperature = thermo3.temperature(RNOMINAL, RREF);
      for(int i = 0; i< numsamples; i++){
        temperature =temperature + (thermo3.temperature(RNOMINAL, RREF));
      }
      temperature = temperature/(numsamples+1);
      break;

    default:
      temperature = 0;
      break;
  }
  
  return temperature;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool checkinternetStatus(){
  int status = Ethernet.maintain();
  bool succesinternetStatus = false;
  switch(status){
    case 0:
      succesinternetStatus = true;
      break;
    case 1:
      succesinternetStatus = true;
      break;
    case 2:
      succesinternetStatus = true;
      break;
    case 3:
      succesinternetStatus = true;
      break;

  }
  return succesinternetStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool updatetimevalues(){
  bool udpatetimevalue = false;
  unsigned long t;
  if(reset == true){
    t = timeClient.getEpochTime();
  }else{
    updatedsecondtimereference();
    t = Globaltime;
  }
    char buff[32];
    sprintf(dayDir,"%02d",day(t));
    sprintf(monthDir,"%02d",month(t));
    sprintf(yearDir,"%02d",year(t));
    sprintf(fileName,"Tempmanzine_%02d_%02d_%02d.log", hour(t), minute(t), second(t));
    sprintf(buff, "%02d.%02d.%02d %02d:%02d:%02d", day(t), month(t), year(t), hour(t), minute(t), second(t));
    Serial.println(buff);
    udpatetimevalue = true;
  return udpatetimevalue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  lcd.begin(20, 4);
  
  thermo0.begin(MAX31865_4WIRE);  // set to 2WIRE or 4WIRE as necessary
  thermo1.begin(MAX31865_4WIRE);  
  thermo2.begin(MAX31865_4WIRE);
  thermo3.begin(MAX31865_4WIRE);

  if(SD.begin(4) == 0)
  {
    Serial.println(F("SD init fail"));          
  }
  while(ShowFreeSpace()<minfreesize){
    deleteOldestFile();
  }


  while (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP")); 
    delay(1000); // retry after 1 sec mantain funciton
  }

  Serial.print(F("IP address: "));
  Serial.println(Ethernet.localIP());
  delay(5000);
  ///// Check to deletfile
  //// ask for time
  timeClient.begin();
  delay(2000);
  timeClient.update();
  delay(5000);

  Globaltime = timeClient.getEpochTime();
  int delaybeforestart=millis();
  lastmillis=lastmillis+delaybeforestart;
  updatedsecondtimereference();

  while(!updatetimevalues()){
    Serial.println(F("Failed to set time values"));
    if(!checkinternetStatus()){
      Serial.println(F("Failed update time due to DHCP"));
    }
    delayfunction(50); 
  }
  while(!ftp.connect(server, user, pass)){
    Serial.println(F("Error connecting to FTP server"));
    delayfunction(10);
  }
  // Restart directory from server
  restartDirectory();
  ftp.stop();
  //////check createsdfile
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  unsigned long currentTime = millis();
  timeClient.update();
  if(currentTime - temperatureTime > temperatureInterval) {
    temperatureTime = currentTime;
    for(int i = 0; i<numsensors;i++){
      if(datachange == false){
        int numsamples = 20;
        float temp = readtemperature(i,numsamples);              //create temperature array for all sensor
        if(!setSDframe(temp,i)){                           //set 1 message for each sensor
          Serial.println(F("Error writing at least 1 frame in SD File"));
        }
      }
    }
  }

 
  if(currentTime - timechange > temperatureInterval/4) {
    timechange = currentTime;
    updatedsecondtimereference();
    unsigned long t = Globaltime;
    char timebufferchange[5];

    sprintf(timebufferchange, "%02d",hour(t));
    if(strcmp(timebufferchange,hours) !=0){
      memcpy(hours,timebufferchange,sizeof(timebufferchange));
      checkinternetStatus();
    }

    updatedsecondtimereference();
    t = Globaltime;
    sprintf(timebufferchange, "%02d",minute(t));
    if((datachange == true)||(strcmp(timebufferchange,min) !=0)){
      datachange = false;
      memcpy(min,timebufferchange,sizeof(timebufferchange));
    /////eliminate files if it is requiered

      if(reset==true){
        reset =false;
      }else{
        while(!ftp.connect(server, user, pass)){
        Serial.println(F("Error connecting to FTP server"));
        delayfunction(100);
        }
        updateDirectory();
        File fh = SD.open(fileName,FILE_READ);
        ftp.store(fileName, fh);
        ftp.stop();
      }
    
      while(!updatetimevalues()){
        Serial.println(F("Failed to set time values"));
        if(!checkinternetStatus()){
          Serial.println(F("Failed update time due to DHCP"));
        }
        delayfunction(50); 
      }
      while(ShowFreeSpace()<minfreesize){ ////
        deleteOldestFile();
      }
      if(!createSDfile(fileName,logFileversion)){
        Serial.println(F("Error creating new file"));
      }
    }
  }  
}



