#include "max6675.h"
#include <AutoPID.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

#define NextionSerial Serial1
#define DebugSerial Serial

#define OUTPUT_PINHTR1 2
#define OUTPUT_PINHTR2 3
#define OUTPUT_PINHTR3 4

// Eeprom Addresses
// 6 adresses for the thermcouple on the cart 
// Adres hieronder voor het sturen van data naar scherm
int TCA1 = 0;
int TCA2 = 2;
int TCA3 = 4;
int TCA4 = 6;
int TCA5 = 8;
int TCA6 = 10;

//Time left intergers
int MAXTemp = 0;
int RampTime = 0;// Will be set after each Speed set
int TotalRampTime = 0;
int TimeSum = 0;

bool TTS = false;

bool Temprecieved = false;
bool Timerecieved = false;

bool FileWithName = false;
bool FileNameCheck = false;

bool TimeLock = false;
bool Dataprocessing = false;
bool ThermoProces = false;

bool SendData = false;

int thermoDO = 38;
int thermoCLK = 40;

//Waardes voor uitlezen van SD-kaart
//SD card Reader
bool check = false;
File myFile; // create file for writing data to SD Card shield.
const int chipSelect = 4;
char NameType[8];

//thermocouples
int thermoCS = 22;
int thermoCS1 = 24;
int thermoCS2 = 26;
int thermoCS3 = 28;
int thermoCS4 = 30;
int thermoCS5 = 32;
int thermoCS6 = 34;
int thermoCS7 = 36;

// Waardes voor lezen en doorsturen naar scherm
//Read Nextion command temp and time
int ReconW = 4;
int FileName = 3;
int TotalChar = 0;

int SetTemp = 0;
unsigned long Time = 0;
int TempHeatUpState = 0;

//Time setting, multiplier
int Minutes = 1;
int MultiplierTime = Minutes * 60;

String readString; //crucial for the decoding of the nextion data
String Name = ""; // string used to save the name of a product, used for the SD card datalog

//time intervals
int T1 = 0;
int T2 = 0;
int T3 = 0;
int T4 = 0;
int T5 = 0;
int T6 = 0;
int T7 = 0;
int T8 = 0;
int T9 = 0;
int T10 = 0;
int T11 = 0;
int T12 = 0;
int T13 = 0;
int T14 = 0;


//Temp intervals
int Temp1 = 0;
int Temp2 = 0;
int Temp3 = 0;
int Temp4 = 0;
int Temp5 = 0;
int Temp6 = 0;
int Temp7 = 0;
int Temp8 = 0;
int Temp9 = 0;
int Temp10 = 0;
int Temp11 = 0;
int Temp12 = 0;
int Temp13 = 0;
int Temp14 = 0;

String CSF;
String NTSF;
String TLF;

//Boolean
boolean TEMPR = false;
boolean TIMR = false;

//Speed setting
int SpeedSet = 1;

//RunningState
int RunningState = 0;
int ThermoCheck = 0;
int Currentstate = 0;

//Delta Time for waiting time heater
unsigned long deltaTime = 0;
unsigned long WriteSDDelta = 0;
unsigned long SendTemp = 0;
unsigned long UpdateThermo = 0;

unsigned long ToTimeLeft = 0;

//Time Left Function
int TimeLeftRun = 0;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
MAX6675 thermocouple1(thermoCLK, thermoCS1, thermoDO);
MAX6675 thermocouple2(thermoCLK, thermoCS2, thermoDO);
MAX6675 thermocouple3(thermoCLK, thermoCS3, thermoDO);
MAX6675 thermocouple4(thermoCLK, thermoCS4, thermoDO);
MAX6675 thermocouple5(thermoCLK, thermoCS5, thermoDO);
MAX6675 thermocouple6(thermoCLK, thermoCS6, thermoDO);
MAX6675 thermocouple7(thermoCLK, thermoCS7, thermoDO);

const int RunningAverageCount = 8;
float RunningAverageBuffer[RunningAverageCount];
int NextRunningAverage;

// Thermocouple Calibration
int ThermoVal1 = 0;
int ThermoVal2 = 0;
int ThermoVal3 = 0;
int ThermoVal4 = 0;
int ThermoVal5 = 0;
int ThermoVal6 = 0;

#define KP 1.3
#define KI 0.2// was 0,3
#define KD 1

double temperature = 20;
double outputVal = 10;
double bangOn = 6;
double bangOff = 1;
double Setpoint = 0;

//thermocouples corrections
double ThermoCorrection = -7;
double ThermoCorrection1 = -12;

float ThermoCorrection2 = 0;
float ThermoCorrection3 = 0;
float ThermoCorrection4 = 0;
float ThermoCorrection5 = 0;
float ThermoCorrection6 = 0;
float ThermoCorrection7 = 0;

float outputMin = 0;
float outputMax = 80;

//Time left
long TL = 0;

int TimeaddC = 0;
int TAC = 0;

//State Nextion
int NTS = 0;
int CS = 0;

AutoPID myPID(&temperature, &Setpoint, &outputVal, outputMin, outputMax , KP, KI, KD);
/// Alles hierboven zijn allemaal waardes die gebruitk worden in de code

String b;

void sendCmd(String cmd)
{
  NextionSerial.print(cmd);
  NextionSerial.write("\xFF\xFF\xFF");
}

void setup()
{

  analogWrite(OUTPUT_PINHTR1, 0);
  analogWrite(OUTPUT_PINHTR2, 0);
  analogWrite(OUTPUT_PINHTR3, 0);

  /// Start serial ports
  /// Wordt gebruikt voor debuggen op een baudrate van 9600
  DebugSerial.begin(9600);
  // Wordt gebruikt voor het versturen van informatie baudrate 115200
  NextionSerial.begin(115200);

  // Load Thermocouple Offsets form EEPROM including negative values
  /// EEPROM betekent  (Electrically Erasable Programmable Read-Only Memory), zit niet meer op de GIGA R1. Haalt eigenlijk opgeslagen data op.
  ThermoVal1 = (int8_t)EEPROM.read(TCA1);
  ThermoVal2 = (int8_t)EEPROM.read(TCA2);
  ThermoVal3 = (int8_t)EEPROM.read(TCA3);
  ThermoVal4 = (int8_t)EEPROM.read(TCA4);
  ThermoVal5 = (int8_t)EEPROM.read(TCA5);
  ThermoVal6 = (int8_t)EEPROM.read(TCA6);
  
  /// Correctie van thermocouples
  ThermoCorrection2 = ThermoVal1 * 0.25;
  ThermoCorrection3 = ThermoVal2 * 0.25;
  ThermoCorrection4 = ThermoVal3 * 0.25;
  ThermoCorrection5 = ThermoVal4 * 0.25;
  ThermoCorrection6 = ThermoVal5 * 0.25;
  ThermoCorrection7 = ThermoVal6 * 0.25;
  
  /// Doorgeven van offset naar Scherm
  DebugSerial.println("OffSet form EEPROM for Thermocouple 2: ");
  DebugSerial.println(ThermoVal1);
  DebugSerial.println(ThermoCorrection2);
  DebugSerial.println("OffSet form EEPROM for Thermocouple 3: ");
  DebugSerial.println(ThermoVal2);
  DebugSerial.println(ThermoCorrection3);
  DebugSerial.println("OffSet form EEPROM for Thermocouple 4: ");
  DebugSerial.println(ThermoVal3);
  DebugSerial.println(ThermoCorrection4);
  DebugSerial.println("OffSet form EEPROM for Thermocouple 5: ");
  DebugSerial.println(ThermoVal4);
  DebugSerial.println(ThermoCorrection5);
  DebugSerial.println("OffSet form EEPROM for Thermocouple 6: ");
  DebugSerial.println(ThermoVal5);
  DebugSerial.println(ThermoCorrection6);
  DebugSerial.println("OffSet form EEPROM for Thermocouple 7: ");
  DebugSerial.println(ThermoVal6);
  DebugSerial.println(ThermoCorrection7);

  //Setup PID Bang Bang
  /// Een PID is een controlleur die constant errors bijhoud en errors verminderd
  /// Bang Bang is een uit/aan controller die wel of geen signaal geeft aan de hand van de variabele
  myPID.setBangBang(bangOn, bangOff);
  //set PID update interval to 250ms (meet om de 250 ms)
  myPID.setTimeStep(250);

  /// Checkt hier of SD geinstalleerd is
  pinMode(53, OUTPUT); //To check if the micro SD is installed
 
  /// Naar scherm sturen dat SD installeerd is
  DebugSerial.print("Initializing SD card...");

  /// De loop om een cure te starten. Eerst wachten op scherm, dan start sturen na scherm nadat de cure is geladen. De loop blijft gewoon doordraaien ook is het nextion serial niet ready
  /// Nextion serial is de data uit de het scherm die de presets in geheugen heeft.
  /// Weergeeft nog een aantal laad texten. Best dom dat de loop blijft doordraaien, ook als het scherm nog niet ingeladen is gaat die al draaien wat niet lekker werkt in de praktijk.
  while (!NextionSerial)
  {
    DebugSerial.println("Waiting on Nextion Screen");
  }

  sendCmd("ArduinoV.txt = \"Blue Mosque\"");
  DebugSerial.println("Sending start up to Nextion Screen");
  delay(500);
  sendCmd("Trigger.val=1");
  sendCmd(""); // clear the buffer
  sendCmd("LiveTemp.txt=\"Initializing\"");
  sendCmd(""); // clear the buffer
  delay(100);
  
  /// wacht op  start commando (moet single source worden vindt ik)
  DebugSerial.println("Awaiting Command of the Nextion Screen or DebugSerial");

  /// Checkt op SD-kaart
  //Nextion recieve data Temp and Time
  TotalChar = ReconW + FileName;
  if (SD.begin())
  {
    DebugSerial.println("SD card is ready to use.");
    sendCmd("page1.SDcardState.txt=\"SD card istalled\"");
  }
  else
  {
    DebugSerial.println("SD card initialization failed");
    sendCmd("page1.SDcardState.txt = \"No SD card istalled\"");
    FileNameCheck = true;
  }
  
/// Code hieronder zorgt ervoor dat deze waardes gebruikt worden in een LOOP en slaat data op van deze variabele in de LOOP
}
void loop()
{
  unsigned long currentMillis = millis();//Start Timer

  String mytext;
  String myvalue;
  String TempvalueAVR;
  String Tempvalue;
  String Tempvalue1;
  String Tempvalue2;
  String Tempvalue3;
  String Tempvalue4;
  String Tempvalue5;
  String Tempvalue6;
  String Tempvalue7;
  String Tempvalue8;

  String Tempval2;
  String Tempval3;
  String Tempval4;
  String Tempval5;
  String Tempval6;
  String Tempval7;

  String TL;
  String NTS;
  String CS;
  String message;
  String Processstate;

  // Thermocouple offset
  String TempOffSet1;
  String TempOffSet2;
  String TempOffSet3;
  String TempOffSet4;
  String TempOffSet5;
  String TempOffSet6;

  String OffSetN1;
  String OffSetN2;
  String OffSetN3;
  String OffSetN4;
  String OffSetN5;
  String OffSetN6;

  String THC2;
  String THC3;
  String THC4;
  String THC5;
  String THC6;
  String THC7;

  /// Dit hieronder checkt de data van het scherm, haalt het door de Debug en weergeeft "Processing Data"
  if (NextionSerial.available() > 0) {

    if (Dataprocessing == false) {
      DebugSerial.print("Dataproccesing state is: ");
      DebugSerial.println(Dataprocessing);
      DebugSerial.print("FileNameCheck state is: ");
      DebugSerial.println(FileNameCheck);
    }
    /*  DebugSerial.println("OffSet form EEPROM for Thermocouples: ");
      DebugSerial.println(ThermoCorrection4);
      DebugSerial.println(ThermoCorrection5);
      DebugSerial.println(ThermoCorrection6);
      DebugSerial.println(ThermoCorrection7); */

    DebugSerial.println("Incoming Data form nextion: ");
    DebugSerial.println(readString);

    if (Dataprocessing == true && ThermoProces == false) {
      sendCmd("t16.txt=\"Processing Data\"");
    }

    /// Kijkt of die een specifieke waarde krijgt volgt bij Z en bepaald hieraan of de calibratie moet beginnen
    char c = NextionSerial.read();  //gets one byte from serial buffer

    if (c == '.') {
      if ('Z') {
        DebugSerial.println("Start Thermocouple Calibration process");
        ThermoProces = true;
        DebugSerial.print("Thermoproces state: ");
        DebugSerial.println(ThermoProces);
      }
    }
    
    /// wacht totdat de byte "C" binnen komt en gelijk is aan "$"
    /// slaat dan tijdelijk data van scherm op als "Trash"
    /// Processes dan de data en stuur dit naar het scherm
    if (c == '$') {
      String Trash = readString;
      DebugSerial.print("The recieved Trash: ");
      DebugSerial.println(Trash);
      Trash = "";

      //if (NextionSerial.available() == 0)//

      DebugSerial.println("Nextion Serial Buffer is empty");
      sendCmd("SendTime.val=1");
      sendCmd("t16.txt=\"Recieved Data\"");
      Dataprocessing = true;

      readString += c; //makes the string readString
    }
    
    /// kijkt of de "C" die binnekomt gelijk is aan "!" en maakt de "Trash data" leeg
    if (c == '!') {
      String Trash = readString;
      DebugSerial.print("The recieved Trash: ");
      DebugSerial.println(Trash);
      Trash = "";
      readString = ""; //clears variable for new input
    }
    
    /// kijkt of "C" die binnenkomt gelijk is aan "x" en herkent dat er geen filename is gegeven. Maakt hierna de data schoon voor nieuwe data.
    if (c == 'x') {
      DebugSerial.println("No File Name has been given");
      FileNameCheck = true;
      //Dataprocessing = false;
      //sendCmd("StopSender.val=0");
      readString = ""; //clears variable for new input
    }
    
    /// Wordt nu niet meer gebruikt voor communicatie (gek dat die er nog wel in staat?)
    if (c == '>') { //Not using fro the new communication, used for hand shake methode -> Force data all the time
      DebugSerial.println("Recieved Nexttion Data request");
      SendData = true;
      readString = ""; //clears variable for new input
    }
    
    /// Een extra manier om de oven de laten draaien zonder file name (in de praktijk werkt deze niet, je hebt altijd een lognaam nodig), onnodig in mijn optiek.
    if (c == '(') {
      FileWithName = false;
      DebugSerial.print("The up comming Run doesn't require a file name ");
    }
    
   /// De cure heeft wel een filename nodig (snap niet waarvoor dit nodig is, werkt toch niet)
    if (c == ')') {
      FileWithName = true;
      DebugSerial.print("The up comming Run requires a File Name: ");
      DebugSerial.print(FileWithName);
    }
    
   /// Stop Commando om de cure te stoppen (beetje ingewikkeld)
    if (c == '@') {
      sendCmd("StopSender.val=0"); //Send to Nextion the running state
      DebugSerial.println("A Stop command has been isued");
      sendCmd("RunningState.txt=\"Stop\""); //Send to Nextion the running state

      readString = ""; //clears variable for new input
      if (check == true) {
        myFile.println("Programm Has been stopped");
        myFile.close(); // if There is a file open it will close it
        readString = ""; //clears variable for new input
        check = false;
      }
      sendCmd("t16.txt=\" \"");
      TempHeatUpState = 15; //Stop command
    }
    
    /// Start het lees en update proces aan het begin de cure cylce.
    /// zet dataprocessen aan, convert letters naar cijfers, zegt data recieved, en update de tijdwaardes en temparatuurwaardes van de thermocouples bij "False". Bij "True" skipt die dit allemaal en start gewoon.
    if (c == ',') {
      Dataprocessing = true;
      while (readString.length() > 1) {
        DebugSerial.print("The Following Data will be processed: ");
        DebugSerial.println(readString); //prints string to serial port out
        int n = readString.toInt();  //convert readString into a number
        DebugSerial.print("Writing Recieved data: ");
        DebugSerial.println(n);
        if (TimeLock == false); {
          if (readString.indexOf('a') > 0) T1 = n;
          if (readString.indexOf('b') > 0) T2 = n;
          if (readString.indexOf('c') > 0) T3 = n;
          if (readString.indexOf('d') > 0) T4 = n;
          if (readString.indexOf('e') > 0) T5 = n;
          if (readString.indexOf('f') > 0) T6 = n;
          if (readString.indexOf('g') > 0) T7 = n;
          if (readString.indexOf('h') > 0) T8 = n;
          if (readString.indexOf('i') > 0) T9 = n;
          if (readString.indexOf('j') > 0) T10 = n;
          if (readString.indexOf('k') > 0) T11 = n;
          if (readString.indexOf('l') > 0) T12 = n;
          if (readString.indexOf('m') > 0) T13 = n;
          if (readString.indexOf('n') > 0) T14 = n;
          if (readString.indexOf('o') > 0) {
            Timerecieved = true;
            sendCmd("SendT.val=1");
            sendCmd("SendTime.val=0");
            TimeLock = false;
          }
        }
        if (readString.indexOf('A') > 0) Temp1 = n;
        if (readString.indexOf('B') > 0) Temp2 = n;
        if (readString.indexOf('C') > 0) Temp3 = n;
        if (readString.indexOf('D') > 0) Temp4 = n;
        if (readString.indexOf('E') > 0) Temp5 = n;
        if (readString.indexOf('F') > 0) Temp6 = n;
        if (readString.indexOf('G') > 0) Temp7 = n;
        if (readString.indexOf('H') > 0) Temp8 = n;
        if (readString.indexOf('I') > 0) Temp9 = n;
        if (readString.indexOf('J') > 0) Temp10 = n;
        if (readString.indexOf('K') > 0) Temp11 = n;
        if (readString.indexOf('L') > 0) Temp12 = n;
        if (readString.indexOf('M') > 0) Temp13 = n;
        if (readString.indexOf('N') > 0) Temp14 = n;
        if (readString.indexOf('O') > 0) {
          Temprecieved = true;
          sendCmd("vis b5,1");
          sendCmd("t16.txt=\" \"");
          Dataprocessing = false;
          sendCmd("StopSend.val=0");
        }
        
        /// Als die ":" binnen krijgt stuurt die de speedset naar het scherm en past deze aan op thermocouples hoe snel deze mogen stijgen volgens de Speedset (Ramptime)
        if (readString.indexOf(':') > 0) {
          SpeedSet = n;
          DebugSerial.print("SpeedSet: ");
          DebugSerial.println(SpeedSet);
          Dataprocessing = false;
        }
        
        /// Bij het ontvangne van "#" zal deze de data loggen op de "Name" van de log en deze omzetten naar een ".TXT" bestand. 
        /// Daarna checkt die of de bestandsnaam al bestaat en geeft 'Error' als deze al bestaat.
        /// Anders zegt die dat er geen "File name exists" en maakt de variabele leeg voor nieuwe naam.
        if (readString.indexOf('#') > 0 && FileWithName == true) {
          if (readString.indexOf('#')) Name = readString;
          DebugSerial.print("The recieved name is: ");
          DebugSerial.println(Name);
          int LastIndex = Name.length() - 1;
          Name.remove(LastIndex);
          Name.concat(".TXT");
          DebugSerial.println(Name);

          Name.toCharArray(NameType, Name.length() + 1);
          if (SD.exists(Name)) {// Checking if The Name of the File is already occupied
            sendCmd("page NameConf"); // Opens error page on Nextion
            DebugSerial.println("File name exists.");
            FileNameCheck = false;
          }

          else {
            DebugSerial.println("File name doesn't exist.");
            DebugSerial.println(NameType);
            myFile = SD.open(Name, FILE_WRITE);
            myFile.print("This file originatie from this file name: ");
            myFile.println(Name);

            FileNameCheck = true;
            check = true;

            readString = ""; //clears variable for new input
          }

          Dataprocessing = false;
        }
        readString = ""; //clears variable for new input
      }
    }
    else {
      readString += c; //makes the string readString
    }
  }
  
/// Thermocouple Offset op het scherm aanpassen en weergeven via + en - knoppen met stappen van 0,25
  while (ThermoProces == true && Dataprocessing == false) {

    char c = NextionSerial.read();

    //TempOffSet1 = int(thermocouple4.readCelsius() - ThermoCorrection4);

    TempOffSet1 = int(thermocouple2.readCelsius() + ThermoCorrection2);
    TempOffSet2 = int(thermocouple3.readCelsius() + ThermoCorrection3);
    TempOffSet3 = int(thermocouple4.readCelsius() + ThermoCorrection4);
    TempOffSet4 = int(thermocouple5.readCelsius() + ThermoCorrection5);
    TempOffSet5 = int(thermocouple6.readCelsius() + ThermoCorrection6);
    TempOffSet6 = int(thermocouple7.readCelsius() + ThermoCorrection7);

    THC2 = ThermoVal1 * 0.25;
    THC3 = ThermoVal2 * 0.25;
    THC4 = ThermoVal3 * 0.25;
    THC5 = ThermoVal4 * 0.25;
    THC6 = ThermoVal5 * 0.25;
    THC7 = ThermoVal6 * 0.25;
    
    OffSetN1 = "\"" + THC2 + "\"";
    OffSetN2 = "\"" + THC3 + "\"";
    OffSetN3 = "\"" + THC4 + "\"";
    OffSetN4 = "\"" + THC5 + "\"";
    OffSetN5 = "\"" + THC6 + "\"";
    OffSetN6 = "\"" + THC7 + "\"";

    // current temperature with compensation

    sendCmd("OffSet1.txt=" + OffSetN1);
    sendCmd("OffSet2.txt=" + OffSetN2);
    sendCmd("OffSet3.txt=" + OffSetN3);
    sendCmd("OffSet4.txt=" + OffSetN4);
    sendCmd("OffSet5.txt=" + OffSetN5);
    sendCmd("OffSet6.txt=" + OffSetN6);
    
    sendCmd("THCC1.val=" + TempOffSet1);
    sendCmd("THCC2.val=" + TempOffSet2);
    sendCmd("THCC3.val=" + TempOffSet3);
    sendCmd("THCC4.val=" + TempOffSet4);
    sendCmd("THCC5.val=" + TempOffSet5);
    sendCmd("THCC6.val=" + TempOffSet6);

    if (c == 'A') {
      ThermoVal1 ++;
      DebugSerial.println("Increasing First Thermocouple");
      ThermoCorrection2 = ThermoVal1 * 0.25;
      sendCmd("OffSet1.txt=" + OffSetN1);
    }
    if (c == 'a') {
      ThermoVal1 --;
      DebugSerial.println("Decreasing First Thermocouple");
      ThermoCorrection2 = ThermoVal1 * 0.25;
      sendCmd("OffSet1.txt=" + OffSetN1);
    }
    if (c == 'B') {
      ThermoVal2 ++;
      DebugSerial.println("Increasing Second Thermocouple");
      ThermoCorrection3 = ThermoVal2 * 0.25;
      sendCmd("OffSet2.txt=" + OffSetN2);
    }
    if (c == 'b') {
      ThermoVal2 --;
      DebugSerial.println("Decreasing Second Thermocouple");
      ThermoCorrection3 = ThermoVal2 * 0.25;
      sendCmd("OffSet2.txt=" + OffSetN2);
    }
    if (c == 'C') {
      ThermoVal3 ++;
      DebugSerial.println("Increasing Third Thermocouple");
      ThermoCorrection4 = ThermoVal3 * 0.25;
      sendCmd("OffSet3.txt=" + OffSetN3);
    }
    if (c == 'c') {
      ThermoVal3 --;
      DebugSerial.println("Decreasing Third Thermocouple");
      ThermoCorrection4 = ThermoVal3 * 0.25;
      sendCmd("OffSet3.txt=" + OffSetN3);
    }
    if (c == 'D') {
      ThermoVal4 ++;
      DebugSerial.println("Increasing Fourth Thermocouple");
      ThermoCorrection5 = ThermoVal4 * 0.25;
      sendCmd("OffSet4.txt=" + OffSetN4);
    }
    if (c == 'd') {
      ThermoVal4 --;
      DebugSerial.println("Decreasing Fourth Thermocouple");
      ThermoCorrection5 = ThermoVal4 * 0.25;
      sendCmd("OffSet4.txt=" + OffSetN4);
    }
    
    if (c == 'E') {
      ThermoVal5 ++;
      DebugSerial.println("Increasing Fifth Thermocouple");
      ThermoCorrection6 = ThermoVal5 * 0.25;
      sendCmd("OffSet5.txt=" + OffSetN5);
    }
    if (c == 'e') {
      ThermoVal5 --;
      DebugSerial.println("Decreasing Fifth Thermocouple");
      ThermoCorrection6 = ThermoVal5 * 0.25;
      sendCmd("OffSet5.txt=" + OffSetN5);
    }

     if (c == 'F') {
      ThermoVal6 ++;
      DebugSerial.println("Increasing Sixth Thermocouple");
      ThermoCorrection7 = ThermoVal6 * 0.25;
      sendCmd("OffSet6.txt=" + OffSetN6);
    }
    if (c == 'f') {
      ThermoVal6 --;
      DebugSerial.println("Decreasing Sixth Thermocouple");
      ThermoCorrection7 = ThermoVal6 * 0.25;
      sendCmd("OffSet6.txt=" + OffSetN6);
    }
      
    // Back button
    if (c == 'X') {
      DebugSerial.println("Back Button has been pressed");

      EEPROM.update(TCA1, ThermoVal1);
      EEPROM.update(TCA2, ThermoVal2);
      EEPROM.update(TCA3, ThermoVal3);
      EEPROM.update(TCA4, ThermoVal4);
      EEPROM.update(TCA5, ThermoVal5);
      EEPROM.update(TCA6, ThermoVal6);

      DebugSerial.println("Data Has been Saved in the EEPROM");
      ThermoProces = false;
      return;
    }

    readString = ""; //clears variable for new input
  }

  /// Actuele thermocouple temparatuur en tijd weergeven op scherm met OFFSET die hieoven is berekend natuurlijk
  if (Dataprocessing == false && ThermoProces == false) {

    if (Timerecieved == true && Temprecieved == true) {

      DebugSerial.println("Data has been recieved");
      DebugSerial.println(T1);
      DebugSerial.println(T2);
      DebugSerial.println(T3);
      DebugSerial.println(T4);
      DebugSerial.println(T5);
      DebugSerial.println(T6);
      DebugSerial.println(T7);
      DebugSerial.println(T8);
      DebugSerial.println(T9);
      DebugSerial.println(T10);
      DebugSerial.println(T11);
      DebugSerial.println(T12);
      DebugSerial.println(T13);
      DebugSerial.println(T14);
      DebugSerial.println("Recieved data temperature: ");
      DebugSerial.println(Temp1);
      DebugSerial.println(Temp2);
      DebugSerial.println(Temp3);
      DebugSerial.println(Temp4);
      DebugSerial.println(Temp5);
      DebugSerial.println(Temp6);
      DebugSerial.println(Temp7);
      DebugSerial.println(Temp8);
      DebugSerial.println(Temp9);
      DebugSerial.println(Temp10);
      DebugSerial.println(Temp11);
      DebugSerial.println(Temp12);
      DebugSerial.println(Temp13);
      DebugSerial.println(Temp14);

      TIMR = true;
      TEMPR = true;

      Timerecieved = false;
      Temprecieved = false;
      
      /// Het registreren van actuele Temparatuur en Tijd op de SD kaart.
      if (check == true && FileNameCheck == true) {
        DebugSerial.println("Writing Run setting to Micro SD card");
        myFile.println("This run has been excudeted with the following setting: ");
        myFile.print("Temperature");
        myFile.print(":");
        myFile.println("Time");

        myFile.print(Temp1);
        myFile.print(":");
        myFile.println(T1);
        myFile.print(Temp2);
        myFile.print(":");
        myFile.println(T2);
        myFile.print(Temp3);
        myFile.print(":");
        myFile.println(T3);
        myFile.print(Temp4);
        myFile.print(":");
        myFile.println(T4);
        myFile.print(Temp5);
        myFile.print(":");
        myFile.println(T5);
        myFile.print(Temp6);
        myFile.print(":");
        myFile.println(T6);
        myFile.print(Temp7);
        myFile.print(":");
        myFile.println(T7);
        myFile.print(Temp8);
        myFile.print(":");
        myFile.println(T8);
        myFile.print(Temp9);
        myFile.print(":");
        myFile.println(T9);
        myFile.print(Temp10);
        myFile.print(":");
        myFile.println(T10);
        myFile.print(Temp11);
        myFile.print(":");
        myFile.println(T11);
        myFile.print(Temp12);
        myFile.print(":");
        myFile.println(T12);
        myFile.print(Temp13);
        myFile.print(":");
        myFile.println(T13);
        myFile.print(Temp14);
        myFile.print(":");
        myFile.println(T14);

      }
    }
    
    /// Het berekenen van de gemiddelde temparatuur van de thermocouples = Rawtempature + Buffer = NextRunningAverage.
    /// NextRunningAverage is gelijk aan NextRunningCount (aantal thermocouples) om de loop opnieuw uit te voeren.
    /// Uiteindelijk komt hieruit de RunningAverageTempature
    float RawTemperature = ((thermocouple.readCelsius()+ ThermoCorrection + thermocouple1.readCelsius()+ ThermoCorrection1) / 2);

    RunningAverageBuffer[NextRunningAverage++] = RawTemperature;
    if (NextRunningAverage >= RunningAverageCount)
    {
      NextRunningAverage = 0;
    }
    float RunningAverageTemperature = 0;
    for (int i = 0; i < RunningAverageCount; ++i)
    {
      RunningAverageTemperature += RunningAverageBuffer[i];
    }
    RunningAverageTemperature /= RunningAverageCount;

    DebugSerial.print("Avarage: ");
    DebugSerial.println(RunningAverageTemperature);

    DebugSerial.print("Raw Value Th 1: ");
    DebugSerial.println(thermocouple.readCelsius()+ ThermoCorrection);

    DebugSerial.print("Raw Value Th 2: ");
    DebugSerial.println(thermocouple1.readCelsius()+ ThermoCorrection1);

    DebugSerial.print("Raw Value Th 3: ");
    DebugSerial.println(thermocouple2.readCelsius() + ThermoCorrection2);

    DebugSerial.print("Raw Value Th 4: ");
    DebugSerial.println(thermocouple3.readCelsius() + ThermoCorrection3);

    DebugSerial.print("Raw Value Th 5: ");
    DebugSerial.println(thermocouple4.readCelsius() + ThermoCorrection4);

    DebugSerial.print("Raw Value Th 6: ");
    DebugSerial.println(thermocouple5.readCelsius() + ThermoCorrection5);

    DebugSerial.print("Raw Value Th 7: ");
    DebugSerial.println(thermocouple6.readCelsius() + ThermoCorrection6);

    DebugSerial.print("Raw Value Th 8: ");
    DebugSerial.println(thermocouple7.readCelsius() + ThermoCorrection7);

    /// Checken van of de Waardes (temparatuur en tijd) van de Thermocouples kloppen (groter dan 1 zijn)
    if (TIMR == true && TEMPR == true && FileNameCheck == true) {
      if (T1 >= 1)TimeaddC ++;
      if (T2 >= 1)TimeaddC ++;
      if (T3 >= 1)TimeaddC ++;
      if (T4 >= 1)TimeaddC ++;
      if (T5 >= 1)TimeaddC ++;
      if (T6 >= 1)TimeaddC ++;
      if (T7 >= 1)TimeaddC ++;
      if (T8 >= 1)TimeaddC ++;
      if (T9 >= 1)TimeaddC ++;
      if (T10 >= 1)TimeaddC ++;
      if (T11 >= 1)TimeaddC ++;
      if (T12 >= 1)TimeaddC ++;
      if (T13 >= 1)TimeaddC ++;
      if (T14 >= 1)TimeaddC ++;

      DebugSerial.print("TimeaddC: ");
      DebugSerial.println(TimeaddC);

      RunningState = 1;
      
     /// Rampsetting hieronder (hoe snel de tempatuuur moet stijgen) hoe hoger de Ramp hoe langzamer de stijging
      TIMR = false;
      TEMPR = false;

      if (SpeedSet == 1) {
        RampTime = 1;
      }
      if (SpeedSet == 2) {
        RampTime = 2;
      }
      if (SpeedSet == 3) {
        RampTime = 4;
      }
      
     /// Bereking hoesnel de tempatuur moet stijgen
      MAXTemp = max(Temp14, max(Temp13, max(Temp12, max(Temp11, max(Temp10, max(Temp9, max(Temp8, max(Temp7, max(Temp6, max(Temp5, max(Temp4, max(Temp3, max(Temp2, Temp1)))))))))))));
      Serial.print("Maximum value is: ");
      Serial.println(MAXTemp);

      TotalRampTime = (MAXTemp - 20) / RampTime;
      DebugSerial.print("Total Ramp Time left: ");
      DebugSerial.println(TotalRampTime);
      
      /// Tijdsom voor TOTAL TIME LEFT (die niet werkt)
      TimeSum = T1 + T2 + T3 + T4 + T5 + T6 + T7 + T8 + T9 + T10 + T11 + T12 + T13 + T14;

      ToTimeLeft = currentMillis;

      if (thermocouple.readCelsius() + ThermoCorrection >= 0 && thermocouple1.readCelsius() + ThermoCorrection1 >= 0)
      {
        ThermoCheck = 1;
      }

      if (thermocouple.readCelsius() + ThermoCorrection <= 0 || thermocouple1.readCelsius() + ThermoCorrection1 <= 0)// can be subsituded for a else statment
      {
        ThermoCheck = 0;
        sendCmd("RunningState.txt=\"Thermocouple Problem\""); //Send to Nextion the running state
        myFile.close();

      }

      DebugSerial.println("There is a Thermocouple problem");

    }

    /// Schrijft omde 5000 miliseconden de temparatuur naar de SD-kaart 
    if (RunningState == 1 && ThermoCheck == 1 && SpeedSet != 0) {
      DebugSerial.println("Heating proces in Progress");

      if (currentMillis - WriteSDDelta >= 5000 && check == true) {

        DebugSerial.print("Writing Data to SD Card");
        DebugSerial.println(Name);

        myFile.print(thermocouple2.readCelsius() + ThermoCorrection2);
        myFile.print(":");
        myFile.print(thermocouple3.readCelsius() + ThermoCorrection3);
        myFile.print(":");
        myFile.print(thermocouple4.readCelsius() + ThermoCorrection4);
        myFile.print(":");
        myFile.print(thermocouple5.readCelsius() + ThermoCorrection5);
        myFile.print(":");
        myFile.print(thermocouple6.readCelsius() + ThermoCorrection6);
        myFile.print(":");
        myFile.println(thermocouple7.readCelsius() + ThermoCorrection7);

        WriteSDDelta = currentMillis;
      }
      
     /// Overheating cdoe (werkt niet)
      
      /*if(RunningAverageTemperature>=120)
        TempHeatUpState = 14;
        sendCmd("RunningState.txt=\"OVERHEATING\""); //Send to Nextion the OVERHEATING state
      */

      /// Bepaald het verwarmingsproces en geeft aan per thermocouple hoe warm die is en wat de SETTEMP is. Bij Case14 eindigt die het programma.
      /// Eindigen van programma gebeurd nu soms ineens na data inladen. 
      sendCmd("RunningState.txt=\"Running\""); //Send to Nextion the running state

      switch (TempHeatUpState)
      {

        case 0:
          SetTemp = Temp1;
          Time = T1;
          Currentstate = 1;
          break;

        case 1:
          SetTemp = Temp2;
          Time = T2;
          Currentstate = 2;
          break;

        case 2:
          SetTemp = Temp3;
          Time = T3;
          Currentstate = 3;
          break;

        case 3:
          SetTemp = Temp4;
          Time = T4;
          Currentstate = 4;
          break;

        case 4:
          SetTemp = Temp5;
          Time = T5;
          Currentstate = 5;
          break;

        case 5:
          SetTemp = Temp6;
          Time = T6;
          Currentstate = 6;
          break;

        case 6:
          SetTemp = Temp7;
          Time = T7;
          Currentstate = 7;
          break;

        case 7:
          SetTemp = Temp8;
          Time = T8;
          Currentstate = 8;
          break;

        case 8:
          SetTemp = Temp9;
          Time = T9;
          Currentstate = 9;
          break;

        case 9:
          SetTemp = Temp10;
          Time = T10;
          Currentstate = 10;
          break;

        case 10:
          SetTemp = Temp11;
          Time = T11;
          Currentstate = 11;
          break;

        case 11:
          SetTemp = Temp12;
          Time = T12;
          Currentstate = 12;
          break;

        case 12:
          SetTemp = Temp13;
          Time = T13;
          Currentstate = 13;
          break;

        case 13:
          SetTemp = Temp14;
          Time = T14;
          Currentstate = 14;
          break;

        case 14:
          DebugSerial.println("Done with the Curing Cycle");
          sendCmd("RunningState.txt=\"Finished\"");
          SetTemp = 0;
          Setpoint = 0;
          myFile.println("Finished cycle");
          myFile.close(); // if There is a file open it will close it
          sendCmd("page Finished"); // Opens Finished Page
          RunningState ++;
          break;

        case 15:
          sendCmd("RunningState.txt=\"Done\""); //Send to Nextion the running state
          TIMR = false;
          TEMPR = false;
          SetTemp = 0;
          Setpoint = 0;
          RunningState = 0;
          TempHeatUpState = 0;
          sendCmd("TLN.val=0");
          sendCmd("NTSN.val=0");
          sendCmd("CSN.val=0");
          sendCmd("page1.SelcSet.txt =\"\"");
          sendCmd("page3.SelcSet.txt =\"\"");
          Currentstate = 0;
          deltaTime = 0;
          return;
          break;
      }
      /// Neem de average van de thermocouples temparatuur en vergelijkt deze met de SETTEMP met een marge van 2 graden. Hiermee naar de volgende stage of blijven verwarmen.
      DebugSerial.print("Switch state: ");
      DebugSerial.println(TempHeatUpState);

      DebugSerial.println("Case Status: ");
      DebugSerial.print("Case Set Temperture: ");
      DebugSerial.println(SetTemp);
      DebugSerial.print("Case Set Time: ");
      DebugSerial.println(Time);
      DebugSerial.print("Setpoint: ");
      DebugSerial.println(Setpoint);
      //((thermocouple.readCelsius() + ThermoCorrection) + (thermocouple1.readCelsius() + ThermoCorrection1) + (thermocouple2.readCelsius() + ThermoCorrection2) + (thermocouple3.readCelsius() + ThermoCorrection3) / 4) >= SetTemp + 12 ||
      
     float ThermoTotalAvg = (((thermocouple2.readCelsius() + ThermoCorrection2) + (thermocouple3.readCelsius() + ThermoCorrection3) + (thermocouple4.readCelsius() + ThermoCorrection4) + (thermocouple5.readCelsius() + ThermoCorrection5) + (thermocouple6.readCelsius() + ThermoCorrection6) + (thermocouple7.readCelsius() + ThermoCorrection7)) / 6);
     /// Leest de thermocouples average temp en past hierop aan
      
     if (ThermoTotalAvg >= SetTemp - 2) Setpoint = SetTemp;
     /// vergelijkt met SETTEMP
      
      //if(((thermocouple.readCelsius()+ThermoCorrection)||(thermocouple1.readCelsius()+ThermoCorrection1)||(thermocouple2.readCelsius()+ThermoCorrection2)||(thermocouple3.readCelsius()+ThermoCorrection3) >= SetTemp) Setpoint = SetTemp;

      else   Setpoint = SetTemp + 10; //deze instelling maakt het mogelijk om sneller te verwarmen VB set temp = 40 deze instelling maakt het +5 dus 45 graden.
      /// Zorgt voor sneller verwarmen van oven door SETTEMP met 10 graden te verhogen
      
      if (ThermoTotalAvg >= SetTemp - 2) {
        if (TTS == false)
        {
          Setpoint = SetTemp;
          deltaTime = currentMillis;
          TTS = true;
        }
       
        /// vergelijkt de stagetijd met total tijd en kijkt of die optijd op temparatuur is. Als TTS (Target Temperature Set) false is verwarmt die door, anders naar volgende stage.
        /// Met intteruptie kan dit zorgen voor verwarring.
        
        // if((unsigned long)(currentMillis - deltaTime) >= (Time*MultiplierTime*1000))

        if ((unsigned long)(currentMillis - deltaTime) >= (Time * MultiplierTime * 1000))
        {
          TempHeatUpState++;
          TTS = false;
        }

      }
      
      /// Update de TL (Time left) aan de hand van of de TTS (Target Temperature Set) is bereikt of niet. TL heeft nooit gewerkt.
      if (Time >= 0 && TTS == true)
      {
        TL = (Time - ((currentMillis - deltaTime) / 60000));
        if (TL != TLF) // Only send time left data when the TL changes
        {
          sendCmd("TLN.val=" + TL);
          TLF = TL;
        }
        DebugSerial.print("Time Left till Next stage: ");
        DebugSerial.println(TL);
        /*
                TAC = (TimeaddC * 30) + (T1+T2+T3+T4+T5+T6+T7+T8+T9+T10+T11+T12+T13+T14);
                DebugSerial.print("Total Time est: ");
                DebugSerial.println(TAC); */
      }

      else
      {
        sendCmd("TLN.val=0");
      }

    }

    // Rens hier kan je snelheid wijzegingen doen
    // Max = 255 Nul = 0 
    /// Het aantal signalen naar de Verwarmingelement (bepaald het vermogen maar werkt niet zeer effectief en is per oven element anders)
    if (SpeedSet == 1) {
      RampTime = 1;
      if (Setpoint >= 110) myPID.setOutputRange(0, 180);
      if (Setpoint >= 100) myPID.setOutputRange(0, 160);
      if (Setpoint >= 90) myPID.setOutputRange(0, 140);
      if (Setpoint >= 70) myPID.setOutputRange(0, 120);
      else myPID.setOutputRange(0, 80);
    }
    if (SpeedSet == 2) {
      RampTime = 2;
      if (Setpoint >= 110) myPID.setOutputRange(0, 220);
      if (Setpoint >= 100) myPID.setOutputRange(0, 200);
      if (Setpoint >= 90) myPID.setOutputRange(0, 180);
      if (Setpoint >= 70) myPID.setOutputRange(0, 160);
      else myPID.setOutputRange(0, 140);
    }
    if (SpeedSet == 3) {
      RampTime = 4;
      if (Setpoint >= 110) myPID.setOutputRange(0, 255);
      if (Setpoint >= 100) myPID.setOutputRange(0, 240);
      if (Setpoint >= 90) myPID.setOutputRange(0, 220);
      if (Setpoint >= 70) myPID.setOutputRange(0, 190);
      else myPID.setOutputRange(0, 170);
    }

    else {
      DebugSerial.println("No SpeedSet Selected");
    }
   
    /// Verklaren van variabelen
    temperature = RunningAverageTemperature;
    myPID.run();
    analogWrite(OUTPUT_PINHTR1, outputVal);
    analogWrite(OUTPUT_PINHTR2, outputVal);
    analogWrite(OUTPUT_PINHTR3, outputVal);

    DebugSerial.print("outputVal: ");
    DebugSerial.println(outputVal);

    delay(200);

    message = RunningAverageTemperature;
    TempvalueAVR = int(RunningAverageTemperature);
    Tempvalue =  int(thermocouple.readCelsius() + ThermoCorrection);
    Tempvalue1 = int(thermocouple1.readCelsius() + ThermoCorrection1);
    Tempvalue2 = thermocouple2.readCelsius() + ThermoCorrection2;
    Tempvalue3 = thermocouple3.readCelsius() + ThermoCorrection3;
    Tempvalue4 = thermocouple4.readCelsius() + ThermoCorrection4;
    Tempvalue5 = thermocouple5.readCelsius() + ThermoCorrection5;
    Tempvalue6 = thermocouple6.readCelsius() + ThermoCorrection6;
    Tempvalue7 = thermocouple7.readCelsius() + ThermoCorrection7;

    Tempval2 = "\"" + Tempvalue2 + "\"";
    Tempval3 = "\"" + Tempvalue3 + "\"";
    Tempval4 = "\"" + Tempvalue4 + "\"";
    Tempval5 = "\"" + Tempvalue5 + "\"";
    Tempval6 = "\"" + Tempvalue6 + "\"";
    Tempval7 = "\"" + Tempvalue7 + "\"";
    
    NTS = SetTemp;
    CS = Currentstate;

    mytext = "\"" + message + "\"";
    //myvalue = "\"" + Tempvalue + "\"";

    /// Checken van data en weergeven van data op het scherm.
    /// Ook checkt het en leest het de thermocouple themparaturen uit en zet deze om naar strings
    /// Het toont ool TL en TLR (Time left to Run) aan de hand van de benoemde berekeningen in minuten
    if (SendData == true && currentMillis - SendTemp >= 1500) //used for Hand shake methode
      //if (currentMillis - SendTemp >= 1500 && Dataprocessing == false)
    {
      TimeLeftRun = (TotalRampTime + TimeSum) - ((currentMillis - ToTimeLeft) / 60000);

      if (TimeLeftRun <= 0) TimeLeftRun = 0;

      DebugSerial.print("Total Time Left: ");
      DebugSerial.println(TimeLeftRun);

      DebugSerial.println("Writing data to Nextion");
      sendCmd("LiveTemp.txt=" + mytext);
      sendCmd("TempverAVR.val=" + TempvalueAVR);
      sendCmd("Tempver.val=" + Tempvalue);
      sendCmd("Tempver1.val=" + Tempvalue1);
      sendCmd("Tempver2.txt=" + Tempval2);
      sendCmd("Tempver3.txt=" + Tempval3);
      sendCmd("Tempver4.txt=" + Tempval4);// The graph can only handle 4 channels
      sendCmd("Tempver5.txt=" + Tempval5);
      sendCmd("Tempver6.txt=" + Tempval6);
      sendCmd("Tempver7.txt=" + Tempval7);
      sendCmd("NTSN.val=" + NTS);
      sendCmd("CSN.val=" + CS);

      sendCmd("TotalTL.val=" + TimeLeftRun);// Send Total time left

      sendCmd("SendCheck.val=1");//Send to the Nextion that new data can be send

      SendTemp = currentMillis;

      readString = ""; //clears variable for new input
      SendData = false;
      sendCmd(""); // clear the buffer
    }
    readString = ""; //clears variable for new input
  }
}
