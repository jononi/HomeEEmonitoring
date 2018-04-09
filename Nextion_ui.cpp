/* Nextion Serial Smart Touch Interface Driver for EE Monitor project
 * Jaafar Benabdallah - 2018
 *
*/

#include "Nextion_ui.h"
#include "mbed.h"

Nextion_UI::Nextion_UI(PinName Tx, PinName Rx) : nextion(Tx, Rx) {
    nextion.baud(115200);
    //TODO add an interruption when enabled touch area is pressed sscanf("\x65\x00%02d\xFF\xFF\xFF",btn_id)
}

void Nextion_UI::init(void) {
    RESET_NEXTION;
    WIFI_ICON_ON;
    SYNC_ICON_OFF;
    UPLOAD_ICON_OFF;
    NO_WAKEUP_ON_SERIALIN;
    WAKEUP_ON_TOUCH;
    SLEEP_ON_NOTOUCH_30S;
}

void Nextion_UI::refresh_clock(struct tm *localt) {
    nextion.printf("hour.txt=\"%02d\"\xff\xff\xff", localt->tm_hour);
    nextion.printf("minute.txt=\"%02d\"\xff\xff\xff", localt->tm_min);
    //nextion.printf("vis sec,1\xff\xff\xff");
    nextion.printf("vis sec,%d\xff\xff\xff", localt->tm_sec % 2 == 0 ? 1 : 0);
}

void Nextion_UI::toggle_wifi_icon(bool wifi_on) {
    nextion.printf("vis wifi,%d\xff\xff\xff", wifi_on? 1 : 0);
}

void Nextion_UI::toggle_sync_icon(bool sync_on) {
    nextion.printf("vis sync,%d\xff\xff\xff", sync_on? 1 : 0);
}

void Nextion_UI::toggle_upload_icon(bool upload_on) {
    nextion.printf("vis upload,%d\xff\xff\xff", upload_on? 1 : 0);
}

void Nextion_UI::toggle_upload_icon(bool save_on) {
    nextion.printf("vis save,%d\xff\xff\xff", save_on? 1 : 0);
}

void Nextion_UI::refresh_A0_txt(int mvolt) {
    nextion.printf("A0_v.txt=\"A0:%d mV\"\xff\xff\xff", mvolt);
}

void Nextion_UI::refresh_A1_txt(int mvolt) {
    nextion.printf("A1_v.txt=\"A0:%d mV\"\xff\xff\xff", mvolt);
}



