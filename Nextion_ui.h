/* Nextion Serial Smart Touch Interface Driver for EE Monitor project
 * Jaafar Benabdallah - 2018
 *
*/

#ifndef NEXTION_UI_H_
#define NEXTION_UI_H_

#include "mbed.h"

/* Nextion display macros */
#define RESET_NEXTION           nextion.printf("rest\xff\xff\xff")
#define WIFI_ICON_ON            nextion.printf("vis wifi,1\xff\xff\xff")
#define WIFI_ICON_OFF           nextion.printf("vis wifi,0\xff\xff\xff")
#define SYNC_ICON_ON	        nextion.printf("vis sync,1\xff\xff\xff")
#define SYNC_ICON_OFF     	    nextion.printf("vis sync,0\xff\xff\xff")
#define UPLOAD_ICON_ON          nextion.printf("vis upload,1\xff\xff\xff")
#define UPLOAD_ICON_OFF         nextion.printf("vis upload,0\xff\xff\xff")
#define NO_WAKEUP_ON_SERIALIN   nextion.printf("usup=0\xff\xff\xff")
#define WAKEUP_ON_TOUCH         nextion.printf("thup=1\xff\xff\xff")
#define SLEEP_ON_NOTOUCH_30S    nextion.printf("thsp=30\xff\xff\xff")
#define WAKE_UP_DISPLAY         nextion.printf("sleep=0\xff\xff\xff")


#define BLACK   0
#define BLUE    31
#define BROWN   48192
#define GREEN   2016
#define YELLOW  65504
#define RED     63488
#define GRAY    33840
#define WHITE   65535

class Nextion_UI {
public:
	Nextion_UI(PinName Tx, PinName Rx);
	virtual ~Nextion_UI();
	void init();
    void toggle_clock(struct tm *localt);
    void toggle_wifi_icon(bool wifi_on);
    void toggle_sync_icon(bool sync_on);
    void toggle_upload_icon(bool save_on);
    void refresh_A0_txt(int mvolt);
    void refresh_A1_txt(int mvolt);


private:
	Serial nextion;
};

#endif /* NEXTION_UI_H_ */
