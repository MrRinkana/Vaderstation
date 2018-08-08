//Object oriented code for diy home weatherstation. ver. 4.0_lcd build

//Code written by Max Bäckström, also utilising code written by: Marshall Taylor @ Sparkfun Electronics, x, y, z, (authors of libraries or example code).
//Code is open source under GNU GENERAL PUBLIC LICENSE v3.0.

//Comments are in Swedish, variables and such in English or in common abbreviations.

//Namngivningsprinciper:
//Variabler och namn på klasser osv. ska vara på Engelska. Gäller ej kommentarer.
//variabler och instanser endast för lokalt bruk (tex. inom en klass) börjar med "_". EX: "_period_s".
//Globala variabler har ej "_" framför. EX: "temperature_c".
//Variabler som ska behandlas (tex snittas) ska heta liknande till variabeln som de hamnar i. EX: "temptoavg_c" och "temp_c". Så att en sökning på slutvariabeln även markerar de innan.
//Enheten en variabel representerar ska anges i slutet av variabelns namn med en "_"som separerar enheten från namnet. EX: "period_s" (enhet sekunder).
//Klasser namnges med STORA BOKSTÄVER, instanser med små. EX: "class BOSSTIMER {}" och "BOSSTIMER timer1;".
/******************************************************************************************************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <SparkFunBME280.h>
#include <Sodaq_DS3231.h>
#include <SD.h>

#include <LCD.h>
#include <LiquidCrystal_I2C.h>

//Inställningar

//För lcdn
#define dispI2Caddr 0x3F 

//För BME280
#define bme280I2Caddr 0x76 //För bme280
#define bmeTimerFirst_s 0
#define bmeTimerloop_s 5

#define NumberOfReadsToAVG 60 //Hur många läsningar som snittas innan värdet loggas. 

#define SDcardCSpin 4


//Information
bool SDloaded;
bool BMEnewReading;
byte failedWriteToSD = 0; //Antal misslyckade skrivningar till SD'n sedan den senaste lyckade.

//Resterande variabler
float temp_C;
float pressure_Pa;
float relHumidity;



//Skapa en instans (internt i klassen som behöver): "BOSSTIMER _namnpåtimer;"
//I klassens setup måste "_namnpåtimer.setup(tid_till_första_aktivering, periodens_längd);" (båda argumenten i sekunder) anropas.
//_namnpåtimer.isExpired() är sann när det är dags
//Efter att uppgiften är utförd måste "_namnpåtimer.reset();" anropas.
class BOSSTIMER {
  unsigned long _period_s;
  unsigned long _expires_ms;

  void internal_reset(unsigned long offset_s){
    _expires_ms = millis() + offset_s * 1000;
  }

  public:

  void setup(unsigned long firstPeriod_s, unsigned long period_s) {
    _period_s = period_s;
    internal_reset(firstPeriod_s);
  }
  boolean isExpired() {
    return _expires_ms <= millis();
  }
  void reset() {
  internal_reset(_period_s);
  }
};
 
//"InstansAvKlass.setup();" måste anropas vid programstart.
//"InstansAvKlass.loop();" måste anropas i programloopen.
class BMESENSOR {
  
  BOSSTIMER _timer;
  BME280 mySensor;
  
  public:
  
    void setup() {
      /**********BME280-inställningar*********************************/
      // Adress- och koummunikationsmetod:
      // commInterface can be I2C_MODE or SPI_MODE
      // specify chipSelectPin using arduino pin names
      // specify I2C address.  Can be 0x77(default) or 0x76
      // For I2C, enable the following and disable the SPI section
      mySensor.settings.commInterface = I2C_MODE;
      mySensor.settings.I2CAddress = bme280I2Caddr; //Ska vara 0x76
      // For SPI enable the following and dissable the I2C section
      // mySensor.settings.commInterface = SPI_MODE;
      // mySensor.settings.chipSelectPin = 10;


      // Körsätt. BOSH rekomenderar FORCED mode för temp läsare men ingen observerad skillnad
      // Sleep = måste väckas
      // Forced = är i ngt hybrid sovläge väcks och gör en läsning
      // Normal = regelbunda avläsningar
  
      // runMode can be:
      //  0, Sleep mode
      //  1 or 2, Forced mode
      //  3, Normal mode
      mySensor.settings.runMode = 3;
      // Ifall NORMAL mode är detta tiden mellan läsningar
      // tStandby can be:
      //  0, 0.5ms
      //  1, 62.5ms
      //  2, 125ms
      //  3, 250ms
      //  4, 500ms
      //  5, 1000ms
      //  6, 10ms
      //  7, 20ms
      mySensor.settings.tStandby = 5;

      // Filter inst. berör endast tryck, motverkar "noise" eller temp luft "puffar"
      // filter can be off or number of FIR coefficients to use:
      //  0, filter off
      //  1, coefficients = 2
      //  2, coefficients = 4
      //  3, coefficients = 8
      //  4, coefficients = 16
      mySensor.settings.filter = 2;

      // Oversampling: Värden kan vara 0 = ingen, 1 till 5 är *1, *2, *4, *8, *16.
      mySensor.settings.tempOverSample = 5;
      mySensor.settings.pressOverSample = 3;
      mySensor.settings.humidOverSample = 5;

      delay(10); //vet ej om denna eller den efter krävs, har båda för säkerhetsskull.
      mySensor.begin(); // Startar BME sensorn samt laddar inställningar
      delay(10); //för säkerhetsskull
      
      //för debugging, glöm inte att "//" tidigare "mySensor.begin();"
      /*
      //Calling .begin() causes the settings to be loaded
      delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
      Serial.println(mySensor.begin(), HEX);
      
      Serial.print("Displaying ID, reset and ctrl regs\n");
      
      Serial.print("ID(0xD0): 0x");
      Serial.println(mySensor.readRegister(BME280_CHIP_ID_REG), HEX);
      Serial.print("Reset register(0xE0): 0x");
      Serial.println(mySensor.readRegister(BME280_RST_REG), HEX);
      Serial.print("ctrl_meas(0xF4): 0x");
      Serial.println(mySensor.readRegister(BME280_CTRL_MEAS_REG), HEX);
      Serial.print("ctrl_hum(0xF2): 0x");
      Serial.println(mySensor.readRegister(BME280_CTRL_HUMIDITY_REG), HEX);
      
      Serial.print("\n\n");
      
      Serial.print("Displaying all regs\n");
      uint8_t memCounter = 0x80;
      uint8_t tempReadData;
      for(int rowi = 8; rowi < 16; rowi++ )
      {
        Serial.print("0x");
        Serial.print(rowi, HEX);
        Serial.print("0:");
        for(int coli = 0; coli < 16; coli++ )
        {
          tempReadData = mySensor.readRegister(memCounter);
          Serial.print((tempReadData >> 4) & 0x0F, HEX);//Print first hex nibble
          Serial.print(tempReadData & 0x0F, HEX);//Print second hex nibble
          Serial.print(" ");
          memCounter++;
        }
        Serial.print("\n");
      }
      
      
      Serial.print("\n\n");
      
      Serial.print("Displaying concatenated calibration words\n");
      Serial.print("dig_T1, uint16: ");
      Serial.println(mySensor.calibration.dig_T1);
      Serial.print("dig_T2, int16: ");
      Serial.println(mySensor.calibration.dig_T2);
      Serial.print("dig_T3, int16: ");
      Serial.println(mySensor.calibration.dig_T3);
      
      Serial.print("dig_P1, uint16: ");
      Serial.println(mySensor.calibration.dig_P1);
      Serial.print("dig_P2, int16: ");
      Serial.println(mySensor.calibration.dig_P2);
      Serial.print("dig_P3, int16: ");
      Serial.println(mySensor.calibration.dig_P3);
      Serial.print("dig_P4, int16: ");
      Serial.println(mySensor.calibration.dig_P4);
      Serial.print("dig_P5, int16: ");
      Serial.println(mySensor.calibration.dig_P5);
      Serial.print("dig_P6, int16: ");
      Serial.println(mySensor.calibration.dig_P6);
      Serial.print("dig_P7, int16: ");
      Serial.println(mySensor.calibration.dig_P7);
      Serial.print("dig_P8, int16: ");
      Serial.println(mySensor.calibration.dig_P8);
      Serial.print("dig_P9, int16: ");
      Serial.println(mySensor.calibration.dig_P9);
      
      Serial.print("dig_H1, uint8: ");
      Serial.println(mySensor.calibration.dig_H1);
      Serial.print("dig_H2, int16: ");
      Serial.println(mySensor.calibration.dig_H2);
      Serial.print("dig_H3, uint8: ");
      Serial.println(mySensor.calibration.dig_H3);
      Serial.print("dig_H4, int16: ");
      Serial.println(mySensor.calibration.dig_H4);
      Serial.print("dig_H5, int16: ");
      Serial.println(mySensor.calibration.dig_H5);
      Serial.print("dig_H6, uint8: ");
      Serial.println(mySensor.calibration.dig_H6);
        
      Serial.println();
      */
      
      
      /***************************************************************/
      
      _timer.setup(bmeTimerFirst_s, bmeTimerloop_s);
    }
    
    void loop() {
      if (_timer.isExpired()) {
        
        _timer.reset();
        
        temp_C = (float)(mySensor.readTempC());              // Temperatur måste läsas först (°C) (kanske skriva om till "mySensor.readFloatTempC();")
        pressure_Pa = (float)(mySensor.readFloatPressure());     // Mät tryck (Pa)
        relHumidity = (float)(mySensor.readFloatHumidity()); // Mät Relativ luftfuktighet (%)
        BMEnewReading = true;
      }
    }
};

//"InstansAvKlass.setup();" måste anropas vid programstart.
//"InstansAvKlass.loop();" måste anropas i programloopen.
LiquidCrystal_I2C lcd(dispI2Caddr, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address (behöver vara utanför classen av lite svåra att förstå anledningar (fråga ola))
class LCD20_4 {

 // Egna karaktärer
  // Grader ( ° )
  byte degree[8] = {
    B00010,
    B00101,
    B00010,
  };


  
  public:
    void setup() {
      lcd.begin(20,4);   // Konfigurerar display som 20 tecken x 4 rader samt slår på bakrundsljus
      delay(1);         // Säkerställa att displayen har startat (borde funka utan, men för säkerhets skull)
      
      lcd.createChar(0, degree);  // Skapar karaktären "grader"
      
      for(int i = 0; i< 2; i++) { // Blinkar 2 gånger med skärmen som "User input" om en reset/uppladdning
        lcd.backlight();
        delay(200);
        lcd.noBacklight();
        delay(200);
      }
      lcd.backlight(); // Avslutar med bakrundsbelysning påslagen
    }
    
    void loop() {
      
      
      
    }
};


class SDLOGGER {
  const int _chipSelect = SDcardCSpin;
  bool _firstNewVal = true;
  byte _currentRead = 0;
  float _tempAVG_C;
  float _relHumidAVG;
  float _pressAVG_Pa;

  public:

    void setup(){
      if (!SD.begin(_chipSelect)){
        SDloaded = false;
      } else{
          SDloaded = true;
        }

      // Markering av en omstart i loggfilen  
      File dataFile = SD.open("datalog.txt", FILE_WRITE); //Öppna eller skapa fil
      if (dataFile) { //Om fil tillgänglig
        dataFile.print("//********************Ny loggning********************//");
        dataFile.println();
        dataFile.close();
      }
    }
  void loop(){
    if (BMEnewReading == true && (_currentRead < NumberOfReadsToAVG)){
      _tempAVG_C += temp_C;
      _pressAVG_Pa += pressure_Pa;
      _relHumidAVG += relHumidity;

      if (!_firstNewVal){
      _tempAVG_C /= 2;
      _pressAVG_Pa /= 2;
      _relHumidAVG /= 2;
      }else{_firstNewVal=false;}
      _currentRead++;
    }
    if (_currentRead == NumberOfReadsToAVG){
      DateTime now = rtc.now(); //Vill ej att denna ska ligga här, måste den det?
      File dataFile = SD.open("datalog.txt", FILE_WRITE);
      if(dataFile){
        //Tid och datum yyyy-mm-dd hh:ss,
        dataFile.print(now.year(), DEC);
        dataFile.print("-");
        if (now.month() < 10){
          dataFile.print("0");
          dataFile.print(now.month(), DEC);
        } else{
            dataFile.print(now.month(), DEC);
          }
        dataFile.print("-");
        if (now.date() < 10){
          dataFile.print("0");
          dataFile.print(now.date(), DEC);
        } else{
            dataFile.print(now.date(), DEC);
          }
        dataFile.print(" "); // Övergång från datum till tid
        if (now.hour() < 10){
          dataFile.print("0");
          dataFile.print(now.hour(), DEC);
        } else{
            dataFile.print(now.hour(), DEC);
          }
        dataFile.print(":");
        if (now.minute() < 10){
         dataFile.print("0");
         dataFile.print(now.minute(), DEC);
        } else{
            dataFile.print(now.minute(), DEC);
          }
        dataFile.print(",");

        //BME sensor värden
        dataFile.print(_tempAVG_C, 2);
        dataFile.print(",");
        dataFile.print(_pressAVG_Pa, 2);
        dataFile.print(",");
        dataFile.print(_relHumidAVG, 2);
        //dataFile.print(","); // behövs ej tills mer saker ska loggas

        dataFile.println(); // Skapa en ny rad
        dataFile.close(); // Stäng filen

      } else{failedWriteToSD++;} 

        //Nollställ snitten.
      _tempAVG_C = 0;
      _pressAVG_Pa = 0;
      _relHumidAVG = 0;

      _currentRead = 0; //Nolställ räkningen.
    }
  }
};

class RTC {
  public:


  void setup(){
    rtc.begin();
    
  }

  //void onTime(){return }

  //void fixTime(){}

};


BMESENSOR bmeSensor;
LCD20_4 display;

void setup() {
  // put your setup code here, to run once:

Wire.begin();

bmeSensor.setup();
display.setup();
}

void loop() {
  // put your main code here, to run repeatedly:

bmeSensor.loop();
display.loop();

}
