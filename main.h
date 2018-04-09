/* EE_monitor: Electric Energy usage Monitoring and Logging
 * Jaafar Benabdallah - 2018
 *
 * This is a work in progress, currently in the early stage of development
 */

#ifndef MAIN_H_
#define MAIN_H_


#include "mbed.h"
#include "TCPSocket.h"
#include "NTPClient.h"
#include "ISM43362Interface.h"
#include "Nextion_ui.h"


// these includes are not required here since they are already added in other headers
// but I still add them to help the IDE auto-completion feature.
#include "rtos.h"
#include "mbed_events.h"

// set timezone (-5L EST, -4 EDT, +1 TUN)
#define TZ	-4
// hardware timer configuration parametres
// Timer frequency (unit: Hz). Range is min=1Hz, max=32kHz (with default parameters: timer 16 bits, time base freq min 1Hz).
#define TIMER_FREQUENCY_HZ             ((uint32_t) 2000)
/* Time base range from minimum frequency possible (unit: Hz). With timer 16 bits,
maximum frequency possible will be 32000 times this value. */
#define TIMER_FREQUENCY_RANGE_MIN_HZ   ((uint32_t)	1000)


/* -------------------		objects declarations		 ------------- */

// serial console for debug assistance
Serial pc(USBTX, USBRX, 115200);

// wifi communications
ISM43362Interface wifi(MBED_CONF_APP_WIFI_SPI_MOSI, MBED_CONF_APP_WIFI_SPI_MISO, MBED_CONF_APP_WIFI_SPI_SCLK, MBED_CONF_APP_WIFI_SPI_NSS, MBED_CONF_APP_WIFI_RESET, MBED_CONF_APP_WIFI_DATAREADY, MBED_CONF_APP_WIFI_WAKEUP, true);

DigitalOut wifi_led(LED3);// WiFi connected status led
Ticker  WiFi_led_ticker;
Ticker	check_wifi_ticker;
bool time_to_check_wifi = false;

// touch display
Nextion_UI touch_ui(PA_0, PA_1); //i.e. D1, D0 : Serial 4
Ticker	refresh_clock_display_ticker;

// other objects
DigitalOut tim3_led(LED1);
DigitalOut led2(LED2);
InterruptIn btn(USER_BUTTON);

// ADC/TIM/DMA
TIM_HandleTypeDef	mTimHandle;

/* ---------- 		RTOS-related declarations			 ------------- */

// rtos variables
Thread	wifiThread;
Thread	analogReadThread;
//Thread	uiQueueThread(osPriorityLow);
//EventQueue	*uiQueue;
EventQueue	*evQueue = mbed_event_queue(); // use the system shared queue for now
Semaphore rtc_synced(1); // looks like signal need to be set again after a read, back to using a semaphore


#endif /* MAIN_H_ */
