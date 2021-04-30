



#define MAX_DISPLAY_BUFFER_SIZE 5000 // e.g. full height for 200x200

#define MAX_DISPLAY_BUFFER_SIZE 800
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
// select one and adapt to your mapping
GxEPD2_BW<GxEPD2_154_D67, MAX_HEIGHT(GxEPD2_154_D67)> display_e(GxEPD2_154_D67(/*CS=*/ SS, /*DC=*/ 5, /*RST=*/ 6, /*BUSY=*/ 7)); // GDEH0154D67
