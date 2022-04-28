#include <Arduino.h>

// FTP library example
// by Industrial Shields


#include <FTP.h>
#include <SdFat.h>
#include <Ethernet.h>
#include <TimeLib.h>     // for update/display of time
#include <NTPClient.h>
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
String logFileversion = "LogFile Version 2.1";
SdFat SD;
const long minfreesize =  31248200; ///free memory size in kB

////Global varibale for time library
EthernetUDP ntpUDP;
NTPClient timeClient(ntpUDP);
unsigned long temperatureTime = 0;
unsigned long temperatureInterval = 1000; 


////Other Global variables
int numsensors = 4;
unsigned long timechange = 0;
char min[5]="00";
bool reset =  true;

unsigned long long Globaltime=0;
unsigned long long lastmillis=0;


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
      delay(100);
    }
    if(!ftp.changedir(mainDir)){
      return false;
    }else{
      if(!ftp.makedir(yearDir)){
        Serial.println(F("Error creating day directory"));
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
      delay(100);
    }
    if(!ftp.changedir(mainDir)){
      return false;
    }else{
      if(!updatedirectoryYear()){
        return false;
      }else{
        if(!ftp.makedir(monthDir)){
          Serial.println(F("Error creating day directory"));
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
      delay(100);
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
    delay(10);
    while(!updatedirectoryMonth());
    delay(10);
    while(!updatedirectoryDay());
    delay(10);
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

////////////////////////////////////////////////////////////////////////////////////////////////////
String setmessage(float sensor,int num){
  /*if(!timeClient.update()){
    Serial.println(F("Error asking time for sensors"));
  }*/
  unsigned long t = timeClient.getEpochTime();
  char time[16];
  sprintf(time,"%02d_%02d_%02d;", hour(t), minute(t), second(t));
  Serial.println(time);
  String c ="";
  c.concat(String(time));
  c.concat("Sensor_");
  c.concat(String(num));
  c.concat("_mazzine;numbers;temperature;");
  c.concat(String(sensor));
  c.concat(";celsius");
  return c;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool setSDframe(float *temp){
  bool successetSDframe=true;
  for(int i=0;i<numsensors;i++){
    String c = setmessage(temp[i],i+1);
    if(!writeSD(fileName,c)){
      successetSDframe=false;
    }
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
      lastModified = (uint16_t (dir.lastWriteDate) << 16 | dir.lastWriteTime); ///test uint32
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
float readtemperature(float *temp,int num){
  for(int i=0;i<numsensors;i++){
    temp[i]=i+1;
  }
  return *temp;
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
  //if(timeClient.update()){
    unsigned long t = timeClient.getEpochTime();
    char buff[32];
    sprintf(dayDir,"%02d",day(t));
    sprintf(monthDir,"%02d",month(t));
    sprintf(yearDir,"%02d",year(t));
    sprintf(fileName,"Tempmanzine_%02d_%02d_%02d.log", hour(t), minute(t), second(t));
    sprintf(buff, "%02d.%02d.%02d %02d:%02d:%02d", day(t), month(t), year(t), hour(t), minute(t), second(t));
    Serial.println(buff);
    udpatetimevalue = true;
  //}
  return udpatetimevalue;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);

  if(SD.begin(4) == 0)
  {
    Serial.println(F("SD init fail"));          
  }
    ////////////// Prepare SD files
  ///// ClearSD 
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
  unsigned long t = timeClient.getEpochTime();

  delay(2000);
  while(!updatetimevalues()){
    Serial.println(F("Failed to set time values"));
    if(!checkinternetStatus()){
      Serial.println(F("Failed update time due to DHCP"));
    }
    delay(50); 
  }
  while(!ftp.connect(server, user, pass)){
    Serial.println(F("Error connecting to FTP server"));
    delay(10);
  }
  // Restart directory from server
  restartDirectory();
  ftp.stop();
  Serial.println(F("Press w to updte file into directory"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  unsigned long currentTime = millis();
  timeClient.update();
  if(currentTime - temperatureTime > temperatureInterval) {
    temperatureTime = currentTime;
    float temperature[numsensors-1];
    readtemperature(temperature,numsensors);              //create temperature array for all sensor
    if(!setSDframe(temperature)){                           //set 1 message for each sensor
      Serial.println(F("Error writing at least 1 frame in SD File"));
    }
  }
 
  if(currentTime - timechange > temperatureInterval/4) {
    timechange = currentTime;
    unsigned long t = timeClient.getEpochTime();
    char timebufferchange[5];
    sprintf(timebufferchange, "%02d",hour(t));
    if(strcmp(timebufferchange,min) !=0){
      memcpy(min,timebufferchange,sizeof(timebufferchange));
    /////eliminate files if it is requiered
      
      if(reset==true){
        reset =false;
      }else{
        while(!ftp.connect(server, user, pass)){
        Serial.println(F("Error connecting to FTP server"));
        delay(100);
        }
        updateDirectory();
        File fh = SD.open(fileName,FILE_READ);
        ftp.store(fileName, fh);
        ftp.stop();
      }
    

      while(ShowFreeSpace()<minfreesize){ ////
        deleteOldestFile();
      }
      while(!updatetimevalues()){
        Serial.println(F("Failed to set time values"));
        if(!checkinternetStatus()){
          Serial.println(F("Failed update time due to DHCP"));
        }
        delay(50); 
      }
      if(!createSDfile(fileName,logFileversion)){
        Serial.println(F("Error creating new file"));
      }
    }
  }  
}



