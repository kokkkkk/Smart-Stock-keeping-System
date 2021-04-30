#include "SoftwareSerial.h"

#include <GxEPD2_BW.h>
#include "GxEPD2_display_selection.h"

#include <Fonts/FreeMonoBold9pt7b.h>

SoftwareSerial mySerial(10, 11); //RX,TX

float price;
float oldPrice;

char serial_array[6];
int index;

void setup() {
  // put your setup code here, to run once:
  mySerial.begin(115200);
  index = 0;
  price = 0;
  oldPrice = 0;

}

void loop() {
  // put your main code here, to run repeatedly:
  
  char price_r;
  while (mySerial.available()>0)
  {
   if(index < 5)
   {
    price_r = (char)mySerial.read();
    serial_array[index] = price_r;
    index++;
   }
   serial_array[index] = '\0';
   
  }
  if(strlen(serial_array) != 0){
     float price_a = atof(serial_array);
     Serial.print(price_a);
     index = 0;
     serial_array[0] = '\0';

     updatePrice(price_a);
  }
    
}

void updatePrice(float p) {

  if (p < price)
    oldPrice = price;
  else
    oldPrice = 0;

  price = p;
  second_display();
}

void second_display()
{

  
  display_e.init();
  display_e.hibernate();

  display_e.setRotation(1);
  display_e.setFont(&FreeMonoBold9pt7b);
  display_e.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display_e.getTextBounds("price", 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  uint16_t x = ((display_e.width() - tbw) / 2) - tbx;
  uint16_t y = ((display_e.height() - tbh) / 2) - tby;

  display_e.setFullWindow();
  display_e.firstPage();



  do
  {
    display_e.fillScreen(GxEPD_WHITE);
    display_e.setCursor(x, y);

    if (oldPrice == 0)
    {
      display_e.setCursor(10, 100);
      display_e.setTextSize(2.9);
      display_e.print(F("$"));
      display_e.print((price));
    }
    else
    {
      display_e.fillRect(90, 165, 80, 5, GxEPD_BLACK);
      display_e.setCursor(90, 170);
      display_e.setTextSize(0.8);
      display_e.print(F("$"));
      display_e.print((oldPrice));


      int off = abs((((price - oldPrice) / oldPrice) * 100));
      display_e.setCursor(90, 150);
      display_e.setTextSize(1);
      display_e.print((off));
      display_e.print(F("%off"));

      display_e.setCursor(10, 100);
      display_e.setTextSize(2.9);
      display_e.print(F("$"));
      display_e.print((price));

    }
  }
  while (display_e.nextPage());

}
