
#include <EEPROM.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Wire.h>
#include "TouchScreen.h"
#include "RTClib.h"
#include <VirtualWire.h>

RTC_DS1307 rtc;

//For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// These are the four touchscreen analog pins
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 8   // can be a digital pin
#define XP 9   // can be a digital pin

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// Color definitions - in 5:6:5
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0 
#define WHITE           0xFFFF
#define TEST            0x1BF5
#define JJCOLOR         0x1CB6
#define JJORNG          0xFD03
#define LIGHTGREEN      0x33ff33

#define LUNDI      1
#define MARDI      2 
#define MERCREDIE  3
#define JEUDI      4
#define VENDREDIE  5
#define SAMEDIE    6
#define DIMANCHE   7

unsigned char JourActuel = MERCREDIE; // a changer lors du chargement du code

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 309);

int i;
int backlightbox;
int backlight = 3;
int blv;
int oldblv;
int humidity=0;
int temp=0;
int heat_index=0;
int pressure;
int UVindex;
float UV;
char MsgReceived[35]; 
int page = 0;
char prevpage=0;
int barv;
char oldDay = 0;

 /*-----------------------Setup-------------------*/

void setup(void) {
  
  Serial.begin(9600);
  
  tft.begin();
  
 /*--------------------------------------------------*/
  
  pinMode(3, OUTPUT);
  //blv = 255; //Run once then put this line in comment and uncomment the line below
  blv = EEPROM.read(2);
 
  
 /*------------ Initialise RF---------------*/
 
   const int receive_pin = 4;
   const int transmit_en_pin = 3;
   
    vw_set_rx_pin(receive_pin);
    vw_setup(2000);   // Bits per sec
    vw_rx_start();       // Start the receiver PLL running
 
 /*----------------RTC-------------------*/
 
  #ifdef AVR
  Wire.begin();
  #else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
  #endif
  
  rtc.begin();
   
  if (! rtc.isrunning()) 
  {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2015, 8, 06, 10, 55, 0));
   }
   
 /*------------Affichage allumage----------------*/
 
  tft.fillScreen(BLACK);
  tft.setRotation(1);
  tft.setCursor(90, 80);
  tft.setTextSize(4);
  tft.setTextColor(WHITE);
  tft.print("Weather");
  tft.setCursor(90, 120);
  tft.print("Station");
  for(i = 0 ; i <= blv; i+=1) { 
    analogWrite(backlight, i);
    delay(2);
  }
  delay(1000);

 /*---------------Status bar--------------------------*/
 
  tft.fillScreen(BLACK);
  tft.setTextSize(1);
  tft.fillRect(0, 0, 320, 10, JJCOLOR); // status bar
  drawhomeicon(); // draw the home icon
  homescr(); // draw the homescreen
  tft.drawRect(0, 200, 245, 40, WHITE); // message box
}

 /*-------------------------Loop---------------------------------*/

void loop() 
{
 
 /*-------------------------Variable---------------------------*/
 char oldHour  = 25;
 char oldMonth,oldMinute,oldSeconde =61; 
 char oldtemp  = 1;
 char TempMax  = 0;
 unsigned char HeightHum =0;
 unsigned char HeightTemp = 0;
 char HumMax   = 0;
 int  GraphX   = 0;
 char GraphxReset = 0;
 char JourBuff = 0;
 char previouspage =0;
 
 unsigned char HeatLundiX, HumLundiX = 30;
 unsigned char HeatLundiy, HumLundiy = 160;
 unsigned char HeatMardiX, HumMardiX = 30;
 unsigned char HeatMardiy, HumMardiy = 160;
 unsigned char HeatMercrediX, HumMercrediX = 30;
 unsigned char HeatMercrediy, HumMercrediy = 160;
 unsigned char HeatJeudiX, HumJeudiX= 30;
 unsigned char HeatJeudiy, HumJeudiy = 160;
 unsigned char HeatVendrediX, HumVendrediX = 30;
 unsigned char HeatVendrediy, HumVendrediy = 160;
 unsigned char HeatSamediX, HumSamediX = 30;
 unsigned char HeatSamediy, HumSamediy = 160;
 unsigned char HeatDimancheX, HumDimancheX= 30;
 unsigned char HeatDimanchey, HumDimanchey = 160;
 /*-----------------------------------------------------*/ 
 
 while (1)
 {
  
  TSPoint p = ts.getPoint();
  
  /*
    Serial.print("X = "); 
     Serial.print(p.x);
     Serial.print("\tY = "); 
     Serial.print(p.y);
     Serial.print("\tPressure = "); 
     Serial.println(p.z);
     */
    // Scale from ~0->1000 to tft.width using the calibration #'s
    p.x = map(p.x, TS_MINX, TS_MAXX, 0,tft.width() );
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

    //Serial.print("p.y:"); // this code will help you get the y and x numbers for the touchscreen
    //Serial.print(p.y);
    //Serial.print("   p.x:");
    //Serial.println(p.x);

   //-------------------Communication RF---------------------------//
   
   uint8_t buf[VW_MAX_MESSAGE_LEN];
   uint8_t buflen = VW_MAX_MESSAGE_LEN;
   
   //Taking the data from the control base
   if (vw_get_message(buf, &buflen)) 
   {
      
      delay(100);
      int i;
      // Message with a good checksum received, dump it. 
      for (i = 0; i < buflen; i++)
      {            
          // Fill Msg Char array with corresponding 
          // chars from buffer.   
          MsgReceived[i] = char(buf[i]);
          //Serial.print(MsgReceived[i]);
      }
            
      sscanf(MsgReceived, "%d,%d,%d,%d",&humidity, &temp, &pressure, &UVindex); // Converts a string to an array
       
   }
   memset( MsgReceived, 0, sizeof(MsgReceived)); // This line is for reset the StringReceived

   UV = UVindex; 

   //-----------------------Real Time Clock-----------------------------//
    
   DateTime now = rtc.now(); //Lecture des donnees du module RTC
   tft.setTextSize(1);
   tft.setTextColor(WHITE);

   if(oldDay!=now.day())
    {
      tft.fillRect(0, 0, 70, 10, JJCOLOR); // status bar  
      tft.setCursor(60, 1);
      tft.print(now.day(), DEC);
      JourActuel++;
      if(JourActuel > 7)
      {
         JourActuel = LUNDI;   
      }
    }

    /*------------Affichage du jour et stockage des données pour le graph------------*/
    
    HeightHum  = map(humidity,20,100,159, 59);
    HeightTemp = map(temp, 0, 50, 159, 59);
    GraphX= map(JourActuel,1,7,30,210);
    
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    
    tft.setCursor(1, 1);
    switch(JourActuel)
    {
      case LUNDI :
      {
        if(page == 1)
        { 
          if((TempMax != HeightTemp)||(JourActuel != JourBuff)||(previouspage != page))
          { 
          
             tft.fillRect(GraphX-9,95,20,70,BLACK);
             HeightTemp = max(HeightTemp, TempMax);
             HeatLundiX= GraphX;
             HeatLundiy= HeightTemp;   
           } 
        }
           if(page == 4)
          { 
            if((HumMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
            { 
             tft.fillRect(GraphX-9,95,20,70,BLACK);
             HeightHum = max(HeightHum, HumMax);
             HumLundiX= GraphX;
             HumLundiy= HeightHum;
            }   
           }       
       
       tft.print("Lundi");
       break;   
       }
       
      case MARDI :
      {
       if((TempMax != HeightTemp)||(JourActuel != JourBuff)||(previouspage != page))
       {
          if(page == 1)
          {   
             tft.fillRect(GraphX-9,95,20,70,BLACK);
             HeightTemp = max(HeightTemp, TempMax);
             HumMardiX= GraphX;
             HumMardiy= HeightTemp; 
          }
       }
          if(page == 4)
          { 
           if((HumMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
           { 
             tft.fillRect(GraphX-9,95,20,70,BLACK);
             HeightHum = max(HeightHum, HumMax);
             HumMardiX= GraphX;
             HumMardiy= HeightHum;
           }   
             
        }     
       tft.print("Mardi");
       break;    
       
      }
      
      case MERCREDIE :
      {
       if(page == 1)
       {  
        if((TempMax != HeightTemp)||(JourActuel != JourBuff)||(previouspage != page))
        { 
           tft.fillRect(GraphX-9,95,20,70,BLACK);
           HeightTemp = max(HeightTemp, TempMax);
           HumMercrediX= GraphX;
           HumMercrediy= HeightTemp; 
        }
       }
        if(page == 4)
        {  
           if((HumMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
           { 
             tft.fillRect(GraphX-9,95,20,70,BLACK);
             HeightHum = max(HeightHum, HumMax);
             HumMercrediX= GraphX;
             HumMercrediy= HeightHum;
           }   
            
       }    
       tft.print("Mercredie");
       break;    
       
      }
      
      case JEUDI :
      {
        if(page == 1)
        { 
          if((TempMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
          { 
            
             tft.fillRect(GraphX-9,95,20,70,BLACK);
             HeightTemp = max(HeightTemp, TempMax);
             HumJeudiX= GraphX;
             HumJeudiy= HeightTemp; 
            }
        }   
          if(page == 4)
          { 
           if((HumMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
           { 
             tft.fillRect(GraphX-9,95,20,70,BLACK);
             HeightHum = max(HeightHum, HumMax);
             HumJeudiX= GraphX;
             HumJeudiy= HeightHum;
           }   
             
       }    
       tft.print("Jeudi");
       break;    
       
      }
      
      case VENDREDIE :
      {
       if(page == 1)
       {  
        if((TempMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
        { 
           tft.fillRect(GraphX-9,95,20,70,BLACK);
           HeightTemp = max(HeightTemp, TempMax);
           HumVendrediX= GraphX;
           HumVendrediy= HeightTemp; 
        }
       }
        if(page == 4)
          { 
           if((HumMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
           { 
             tft.fillRect(GraphX-9,95,20,70,BLACK);
             HeightHum = max(HeightHum, HumMax);
             HumVendrediX= GraphX;
             HumVendrediy= HeightHum;
           }   
               
       }     
       tft.print("Vendredie");
       break;    
       
      } 
      
      case SAMEDIE :
      {
       if(page == 1)
       {  
        if((TempMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
        { 
           tft.fillRect(GraphX-9,95,20,70,BLACK);
           HeightTemp = max(HeightTemp, TempMax);
           HeatSamediX= GraphX;
           HeatSamediy= HeightTemp; 
        }
       }
        if(page == 4)
        { 
        if((HumMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
        { 
           tft.fillRect(GraphX-9,95,20,70,BLACK);
           HeightHum = max(HeightHum, HumMax);
           HumSamediX= GraphX;
           HumSamediy= HeightHum;
        }   
            
       }    
       tft.print("Samedie");
       break;     
      }
      
      case DIMANCHE :
      {
       if(page == 1)
       {  
        if((TempMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
        { 
           tft.fillRect(GraphX-9,95,20,70,BLACK);
           HeightTemp = max(HeightTemp, TempMax);
           HeatDimancheX= GraphX;
           HeatDimanchey= HeightTemp; 
        }
       }
        if(page == 4)
        { 
           if((HumMax != HeightHum)||(JourActuel != JourBuff)||(previouspage != page))
           { 
             tft.fillRect(GraphX-9,95,20,70,BLACK);
             HeightHum = max(HeightHum, HumMax);
             HumDimancheX= GraphX;
             HumDimanchey= HeightHum; 
           }  
            
       }    
       tft.print("Dimanche");
       break;    
      }
     }
     
     
     tft.setCursor(75, 1);
     tft.print("/");
    
     tft.setCursor(83, 1);
     tft.print(now.month(), DEC); //Affichage du mois

     tft.setCursor(100, 1);
     tft.print("/");
    
     tft.setCursor(108, 1);
     tft.print(now.year(), DEC); //Affichagede l'année
   
    
    if(oldHour!=now.hour())
    {
      tft.fillRect(145, 0, 155, 10, JJCOLOR);  
      tft.setCursor(145, 1);
      tft.print(now.hour(), DEC);              //Affichage de l'heure
    }
    
    tft.setCursor(158, 1);
    tft.print(":");
    
    if(oldMinute!=now.minute())
    {
      tft.fillRect(165, 0, 175, 10, JJCOLOR);   
      tft.setCursor(165, 1);
      tft.print(now.minute(), DEC);            //Affichage des minute  
    }
    
    tft.setCursor(178, 1);
    tft.print(":");
    
    if(oldSeconde!=now.second())
    {
      tft.fillRect(185, 0, 198, 10, JJCOLOR); 
      tft.setCursor(185, 1);
      tft.print(now.second(), DEC);           //Affichage des secondes
    }
    
    oldDay = now.day();
    oldSeconde = now.second();
    oldMinute = now.minute();
    oldHour = now.hour();

   
    if(previouspage != page)
    {

      for(GraphxReset = 30; GraphxReset < 211; GraphxReset+30)
      {
        tft.fillRect(GraphxReset,95,20,70,BLACK);
      }
     
    } 
    
      if(page==1)
      {
        tft.setTextColor(RED);
        tft.setCursor(HeatLundiX, HeatLundiy);
        tft.print(".");
        tft.setCursor(HeatMardiX, HeatMardiy);
        tft.print(".");
        tft.setCursor(HeatMercrediX, HeatMercrediy);
        tft.print(".");
        tft.setCursor(HeatJeudiX, HeatJeudiy);
        tft.print(".");
        tft.setCursor(HeatVendrediX, HeatVendrediy);
        tft.print(".");
        tft.setCursor(HeatSamediX, HeatSamediy);
        tft.print(".");
        tft.setCursor(HeatDimancheX, HeatDimanchey);
        tft.print(".");
        tft.drawLine(10,165,240,165,WHITE);
      }
      
      if(page==4)
      {
        tft.setTextColor(RED);
        tft.setCursor(HumLundiX, HumLundiy);
        tft.print(".");
        tft.setCursor(HumMardiX, HumMardiy);
        tft.print(".");
        tft.setCursor(HumMercrediX, HumMercrediy);
        tft.print(".");
        tft.setCursor(HumJeudiX, HumJeudiy);
        tft.print(".");
        tft.setCursor(HumVendrediX, HumVendrediy);
        tft.print(".");
        tft.setCursor(HumSamediX, HumSamediy);
        tft.print(".");
        tft.setCursor(HumDimancheX, HumDimanchey);
        tft.print(".");
        tft.drawLine(10,165,240,165,WHITE);
      }
    
    /*----------------Bouton---------------------------*/
   
    // area 1
    if (p.y > -10 && p.y < 100 && p.x > 223 && p.x < 286) // if this area is pressed
    { 
      if (page == 0) // if you are on the "home" page (0)
      { 
        page = 1; // then you just went to the first page
        clearcenter();
        redraw(); // redraw the screen with the page value 1, giving you the page 1 menu
      }
    }
    // area 2
    if (p.y > -10 && p.y < 100 && p.x > 140 && p.x < 207) 
    {
      
      if (page == 0) 
      {
        page = 2;
        clearcenter();
        redraw();
      }
    }
    // settings
    if (p.y > -10 && p.y < 100 && p.x > 58 && p.x < 119) 
    {
      
      if (page == 0) 
      {
        page = 6;
        clearcenter();
        redraw();
      }
    }
    // area 4
    if (p.y > 115 && p.y < 254 && p.x > 223 && p.x < 286) 
    {
      
      if (page == 0) 
      {
        page = 4;
        clearcenter();
        redraw();
      }
    }
    // area 5
    if (p.y > 115 && p.y < 254 && p.x > 140 && p.x < 207) 
    {
      
      if (page == 0) 
      {
        page = 5;
        clearcenter();
        redraw();
      }
    }
         
     
     if (p.y > 190 && p.y < 240 && p.x > -14 && p.x < 48)  // if the home icon is pressed
     { 
        if (page == 6)      // if you are leaving the settings page
		{ 
			clearmessage(); // clear the battery voltage out of the message box
			tft.setTextSize(2);
			tft.setTextColor(YELLOW);
			tft.setCursor(12, 213);
			tft.print("Settings Saved"); // display settings saved in message box
			EEPROM.write(2, blv); // write the backlight value to EEPROM, so it will not lose settings without power
			clearsettings(); // erase all the drawings on the settings page
       }
        if (page == 0)       // if you are already on the home page
		{ 
			drawhomeiconred(); // draw the home icon red
			delay(250); // wait a bit
			drawhomeicon(); // draw the home icon back to white
			return; // if you were on the home page, stop.
        }
      else      // if you are not on the settings, home, or keyboard page
	  {
			page = prevpage; // a value to keep track of what WAS on the screen to redraw/erase only what needs to be
			page = 0; // make the current page home
			redraw(); // redraw the page
      }
    }
     // backlight buttons
    if (p.y > -14 && p.y < 30 && p.x > 220 && p.x < 280) 
    {
      
      if (page == 6) 
      {
        blightdown();
      }
    }
    if (p.y > 187 && p.y < 230 && p.x > 220 && p.x < 287) 
    {
    
      if (page == 6) 
      {
        blightup();
      }
    }



    //------------------Affichage des données----------------------//
    if((oldtemp != temp)||(previouspage != page))
    {
      if(page==1)
      {
        tft.fillRect(200,37,250,47,BLACK);
        tft.setTextSize(2);
        tft.setTextColor(WHITE);
        tft.setCursor(185, 37);
        tft.print(":");
        tft.setCursor(200, 37);
        tft.print(temp);
        tft.setCursor(223, 28);
        tft.print((char)248);
        tft.setCursor(236, 37);
        tft.print("C");
      }
      if(page==2)
      {
       tft.fillRect(156,37,250,47,BLACK);
       tft.setTextSize(2);
       tft.setTextColor(WHITE);
       tft.setCursor(160, 37);
       tft.print(pressure);
      }
      if(page==4)
      { 
       tft.fillRect(156,37,250,47,BLACK);
       
       tft.setTextSize(2);
       tft.setTextColor(WHITE);
       tft.setCursor(146, 37);
       tft.print(":");
       tft.setCursor(160, 37);
       tft.print(humidity);
       tft.setCursor(190, 37);
       tft.print((char)37);
      } 
      if(page==5)
      {
       tft.fillRect(156,37,250,47,BLACK);
       tft.setTextSize(2);
       tft.setTextColor(WHITE);
       UV /= 100.0; 
       tft.setCursor(160, 37);
       tft.print(UV);
      } 
    }
	/*--------------Mise a jour des variables----------------*/
	
    oldtemp=temp;
    TempMax = HeightTemp;
    previouspage = page;
    JourBuff = JourActuel;
 }
}

/*----------------------Fonction-----------------------*/

void redraw() { // redraw the page
  if ((prevpage != 6) || (page !=7)) {
    clearcenter();
  }
  if (page == 0) {
    homescr();
  }
  if (page == 1) {
    menu1();
  }
  if (page == 2) {
    menu2();
  }
  if (page == 3) {
    menu3();
  }
  if (page == 4) {
    menu4();
  }
  if (page == 5) {
    menu5();
  }
  if (page == 6) {
    settingsscr();
  }
}

void clearmessage() {
  tft.fillRect(12, 213, 226, 16, BLACK); // black out the inside of the message box
}

void clearcenter() { // the reason for so many small "boxes" is that it's faster than filling the whole thing
  ;
  tft.fillRect(0, 11, 380, 180, BLACK);

}
void clearsettings() { // this is used to erase the extra drawings when exiting the settings page
  tft.fillRect(0, 20, 320, 110, BLACK);
  delay(500);
  clearmessage();
}


void settingsscr() {
  // backlight level
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.fillRect(0, 20, 60, 50, RED);
  tft.drawRect(0, 20, 60, 50, WHITE);
  tft.drawRect(80, 20, 160, 50, JJCOLOR);
  tft.fillRect(260, 20, 60, 50, GREEN);
  tft.drawRect(260, 20, 60, 50, WHITE);
  tft.setCursor(22, 33);
  tft.print("-");
  tft.setCursor(282, 33);
  tft.print("+");
  tft.setTextSize(1);
  tft.setCursor(120, 31);
  tft.print("Backlight Level");
  tft.drawRect(110, 48, 100, 10, WHITE);
  blbar();
  
  /*
  battv = readVcc(); // read the voltage
  itoa (battv, voltage, 10);
  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(12, 213);
  tft.print(voltage);
  tft.setCursor(60, 213);
  tft.print("mV");
  
  battpercent = (battv / 5000) * 100, 2;
  itoa (battpercent, battpercenttxt, 10);
  tft.print(102, 213, battpercenttxt, YELLOW, 2);
  */
}

void blightup() { // increase the backlight brightness
  blv = blv + 5;
  if (blv >= 255) {
    blv = 255;
  }
  analogWrite(backlight, blv);
  blbar();
}
void blightdown() { // decrease the backlight brightness
  blv = blv - 5;
  if (blv <= 5) {
    blv = 5;
  }
  analogWrite(backlight, blv);
  blbar();
}
void blbar() { // this function fills the yellow bar in the backlight brightness adjustment
  if (blv < barv) {
    tft.fillRect(111, 49, 98, 8, BLACK);
  }
  backlightbox = map(blv, 1, 255, 0, 98);
  tft.fillRect(111, 49, backlightbox, 8, YELLOW);
  barv = blv;
  delay(25);
}

void menu1() {
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(50, 37);
  tft.print("Temperature");
  boxetemp();
  tft.setTextColor(CYAN);
  tft.setCursor(1,152);
  tft.print("5");
  tft.setTextColor(BLUE);
  tft.setCursor(1,142);
  tft.print("10");
  tft.setTextColor(ILI9341_GREENYELLOW);
  tft.setCursor(1,132);
  tft.print("15");
  tft.setTextColor(GREEN);
  tft.setCursor(1,122);
  tft.print("20");
  tft.setTextColor(YELLOW);
  tft.setCursor(1,112);
  tft.print("25");
  tft.setTextColor(RED);
  tft.setCursor(1,102);
  tft.print("30");    
}
void menu2() {
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
 
}
void menu3() {
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  
 
}
void menu4() {
  
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(50, 37);
  tft.print("Humidite");
  boxetemp();
  tft.setTextColor(CYAN);
  tft.setCursor(1,152);
  tft.print("30");
  tft.setTextColor(BLUE);
  tft.setCursor(1,142);
  tft.print("40");
  tft.setTextColor(ILI9341_GREENYELLOW);
  tft.setCursor(1,132);
  tft.print("50");
  tft.setTextColor(GREEN);
  tft.setCursor(1,122);
  tft.print("60");
  tft.setTextColor(YELLOW);
  tft.setCursor(1,112);
  tft.print("70");
  tft.setTextColor(RED);
  tft.setCursor(1,102);
  tft.print("80");
 
}
void menu5() {
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  
}
void boxes() { // redraw the button outline boxes
  tft.drawRect(0, 20, 150, 50, JJCOLOR);
  tft.drawRect(170, 20, 150, 50, JJCOLOR);
  tft.drawRect(0, 80, 150, 50, JJCOLOR);
  tft.drawRect(170, 80, 150, 50, JJCOLOR);
  tft.drawRect(0, 140, 150, 50, JJCOLOR);
}
void drawhomeicon() { // draws a white home icon
  tft.drawLine(280, 219, 299, 200, WHITE);
  tft.drawLine(300, 200, 304, 204, WHITE);
  tft.drawLine(304, 203, 304, 200, WHITE);
  tft.drawLine(305, 200, 307, 200, WHITE);
  tft.drawLine(308, 200, 308, 208, WHITE);
  tft.drawLine(309, 209, 319, 219, WHITE);
  tft.drawLine(281, 219, 283, 219, WHITE);
  tft.drawLine(316, 219, 318, 219, WHITE);
  tft.drawRect(284, 219, 32, 21, WHITE);
  tft.drawRect(295, 225, 10, 15, WHITE);
}
void drawhomeiconred() { // draws a red home icon
  tft.drawLine(280, 219, 299, 200, RED);
  tft.drawLine(300, 200, 304, 204, RED);
  tft.drawLine(304, 203, 304, 200, RED);
  tft.drawLine(305, 200, 307, 200, RED);
  tft.drawLine(308, 200, 308, 208, RED);
  tft.drawLine(309, 209, 319, 219, RED);
  tft.drawLine(281, 219, 283, 219, RED);
  tft.drawLine(316, 219, 318, 219, RED);
  tft.drawRect(284, 219, 32, 21, RED);
  tft.drawRect(295, 225, 10, 15, RED);
}

void boxetemp()
{
 tft.drawLine(20,90,20,175,WHITE); 
 
 tft.drawLine(17,90,23,90,WHITE); //  ---
 tft.drawLine(17,90,20,85,WHITE);//  /
 tft.drawLine(22,90,20,85,WHITE);

 tft.drawLine(240,168,240,162,WHITE);
 tft.drawLine(240,168,245,165,WHITE);
 tft.drawLine(240,162,245,165,WHITE);

 tft.drawLine(15,155,20,155,WHITE);
 tft.drawLine(15,145,20,145,WHITE);
 tft.drawLine(15,135,20,135,WHITE);
 tft.drawLine(15,125,20,125,WHITE);
 tft.drawLine(15,115,20,115,WHITE);
 tft.drawLine(15,105,20,105,WHITE);

 tft.setTextSize(1);
  
 tft.drawLine(45,155,45,165,WHITE);
  if(JourActuel == LUNDI)
  {
    tft.setTextColor(RED);
  }
  else
  {
  tft.setTextColor(WHITE);
  }
  tft.setCursor(28,173);
  tft.print("LU");

  tft.drawLine(75,155,75,165,WHITE);
  if(JourActuel == MARDI)
  {
    tft.setTextColor(RED);
  }
  else
  {
  tft.setTextColor(WHITE);
  }
  tft.setCursor(58,173);
  tft.print("MA");       

 tft.drawLine(105,155,105,165,WHITE);
  if(JourActuel == MERCREDIE)
  {
    tft.setTextColor(RED);
  }
  else
  {
  tft.setTextColor(WHITE);
  }
  tft.setCursor(88,173);
  tft.print("ME");
  
 tft.drawLine(135,155,135,165,WHITE);
  if(JourActuel == JEUDI)
  {
    tft.setTextColor(RED);
  }
  else
  {
  tft.setTextColor(WHITE);
  }
  tft.setCursor(118,173);
  tft.print("JE");
  
 tft.drawLine(165,155,165,165,WHITE);
  if(JourActuel == VENDREDIE)
  {
    tft.setTextColor(RED);
  }
  else
  {
  tft.setTextColor(WHITE);
  }
  tft.setCursor(148,173);
  tft.print("VE");
  
 tft.drawLine(195,155,195,165,WHITE);
  if(JourActuel == SAMEDIE)
  {
    tft.setTextColor(RED);
  }
  else
  {
  tft.setTextColor(WHITE);
  }
  tft.setCursor(178,173);
  tft.print("SA");
  
 tft.drawLine(225,155,225,165,WHITE);
  if(JourActuel == DIMANCHE)
  {
    tft.setTextColor(RED);
  }
  else
  {
  tft.setTextColor(WHITE);
  }
  tft.setCursor(208,173);
  tft.print("DI");

  tft.drawLine(10,165,240,165,WHITE);
    
}

void homescr() 
{
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  boxes();
  tft.setCursor(10, 37);
  tft.print("Temperature");
  tft.setCursor(197, 37);
  tft.print("Humidite");
  tft.setCursor(25, 97);
  tft.print("Pression");
  tft.setCursor(210, 97);
  tft.print("Menu 4");
  tft.setCursor(27, 157);
  tft.print("Index UV");
}

