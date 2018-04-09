/* EE_monitor: Electric Energy usage Monitoring and Logging
 * Jaafar Benabdallah - 2018
 *
 * This is a work in progress, currently in the early stage of development
 */

#include "main.h"

/* ----------	 Forward declarations / brief functions  ------------- */

const char *sec2str(nsapi_security_t sec);
int	 scan_demo(WiFiInterface *wifi);
bool get_unix_timestamp(WiFiInterface *wifi_stack);
void wifi_thread_func();
void analog_read_thread_func();
void tim3_thread_func();
void print_voltage(float voltage, uint8_t analog_pin, time_t timestamp);
void print_voltage_int(uint16_t voltage, uint8_t analog_pin, time_t timestamp);
void print_wifi_status(bool wifi_status);
bool wifi_check_status(WiFiInterface *wifi_stack);
static void TIM3_Config(void);
static void TIM3_Error_Handler(void);
void toggle_wifi_led_cb() {
	wifi_led = !wifi_led;
}

void check_wifi_cb() {
	time_to_check_wifi = true;
}

void btn_cb() {
	led2 = !led2;
}

extern "C" void TIM3_IRQHandler(void) {
	HAL_TIM_IRQHandler(&mTimHandle);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	tim3_led = !tim3_led;
}


/* ----------		 Main Function / Thread		 ------------- */

int main()
{
	printf("\r\n***************************************************************\r\n");
	printf("***      EE monitor early prototype - STM32L475 IoT Node    ***\r\n");
	printf("***************************************************************\r\n");

	printf("Main thread context %p\r\n", Thread::gettid());

	wifi_led = 0;
	TIM3_Config();
	btn.fall(btn_cb);

	touch_ui.init();

	wifiThread.start(wifi_thread_func);
	analogReadThread.start(analog_read_thread_func);
	//tim3TaskThread.start(tim3_thread_func);

	evqueue->dispatch_forever();

	/*
	 *
	 */

	/* use an event queue for these two events:
	 * 	- inform threads that rtc is synced
	 *  - when an analog read is performed, post event with the pin name nad the voltage value
	 *  - rewrite the code to use the new esWifi drivers based on Mbed Network API
	 *  - check if the new driver is rtos friendly (old one isn't, spends most of the time polling for rx data) --> it is
	 *  - TODO: when an analog read is performed, add value to a mail data structure
	 */

}


/* ---------- Wifi + time sync Functions  ------------- */

/*		Connectivity and communication thread
 * - establish and maintain wifi connection
 * - send and receive data over wifi (only tcp for now)
 *
 */

void wifi_thread_func()
{
	//operation flags
	bool wifi_connected_flag = false;
	bool rtc_synced_flag = false;

	rtc_synced.wait(); // take the lone token to lock access to rtc time

	while (1) {
		if(!wifi_connected_flag) {
			wifi.reset();
			// scan_demo(&wifi);
			wifi.set_credentials(MBED_CONF_APP_WIFI_SSID_G, MBED_CONF_APP_WIFI_PASSWORD_G, NSAPI_SECURITY_WPA2);
			evqueue->call(printf, "\r\n> Connecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID_G);
//			printf("\r\n> Connecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);
			if (wifi.connect() == 0) {
				wifi_connected_flag = true;
				check_wifi_ticker.attach(&check_wifi_cb, 3.0);
				WiFi_led_ticker.attach(&toggle_wifi_led_cb, 0.4);
			}
			evqueue->call(print_wifi_status, wifi_connected_flag);
		}

		if (wifi_connected_flag && !rtc_synced_flag) {
			if (get_unix_timestamp(&wifi)) {
				rtc_synced_flag = true;
				time_t ts_s = time(NULL);
				printf("> utc time: %s \r\n", ctime(&ts_s));
				time_t lt = (int)ts_s - 3600*5;
				printf("> local time: %s \r\n", ctime(&lt));
				rtc_synced.release();
			}
		}

		// check connection status by dns resolving an external server
		if (time_to_check_wifi) {
			wifi_connected_flag = wifi_check_status(&wifi);
			time_to_check_wifi = false;
		}

		if (wifi_connected_flag) {
			// safe to switch away off this thread
			Thread::yield();
		} else {
			Thread::wait(2000); // wait for 2 sec before retrying to wifi connect
		}
	}
}


bool get_unix_timestamp(WiFiInterface *wifi_stack) {
	bool ret = false;
	NTPClient ntp(wifi_stack);

	time_t ntp_timestamp = ntp.get_timestamp();
	if(ntp_timestamp > 0) {
		set_time(ntp_timestamp);
		printf("> current unix time: %ld\r\n", ntp_timestamp);
		ret = true;
	} else {
		printf("> Error getting ntp time: %ld. Retrying...\r\n", ntp_timestamp);
	}
	return ret;
}


bool wifi_check_status(WiFiInterface *wifi_stack) {
	SocketAddress address;
	wifi_stack->gethostbyname("www.google.com", &address);
	const char *ip_test = address.get_ip_address();
	if (strlen(ip_test) < 9) {
		printf("> No internet connection.\r\n");
		WiFi_led_ticker.detach();
		wifi_led = 0;
		return false;
	}
	return true;
}


const char *sec2str(nsapi_security_t sec)
{
	switch (sec) {
	case NSAPI_SECURITY_NONE:
		return "None";
	case NSAPI_SECURITY_WEP:
		return "WEP";
	case NSAPI_SECURITY_WPA:
		return "WPA";
	case NSAPI_SECURITY_WPA2:
		return "WPA2";
	case NSAPI_SECURITY_WPA_WPA2:
		return "WPA/WPA2";
	case NSAPI_SECURITY_UNKNOWN:
	default:
		return "Unknown";
	}
}


int scan_demo(WiFiInterface *wifi)
{
	WiFiAccessPoint *ap;

	printf("Scan:\n");

	int count = wifi->scan(NULL,0);
	printf("%d networks available.\n", count);

	/* Limit number of network arbitrary to 15 */
	count = count < 15 ? count : 15;

	ap = new WiFiAccessPoint[count];
	count = wifi->scan(ap, count);
	for (int i = 0; i < count; i++)
	{
		printf("Network: %s secured: %s BSSID: %X:%X:%X:%X:%X:%X RSSI: %d Ch: %d\n", ap[i].get_ssid(),
				sec2str(ap[i].get_security()), ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
				ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5], ap[i].get_rssi(), ap[i].get_channel());
	}

	delete[] ap;
	return count;
}


/* ---------- Analog Read functions  ------------- */

void analog_read_thread_func() {
	//!\\ AnalogIn ops are thread safe, they do use mutexes.

	PinName ana_in_l[] = {A0, A1};
	float ana_reading[]= {0, 0};
	float vref = 0;

	AnalogIn analog_input_1(ana_in_l[0]);
	AnalogIn analog_input_2(ana_in_l[1]);

	while(1) {
		// let the ADC start converting from the start to get it warmed up by the time rtc is synced.
		ana_reading[0] = analog_input_1.read(); // Read the analog input value (value from 0.0 to 1.0 = full ADC conversion range)
		ana_reading[1] = analog_input_2.read();

		// don't go further until confirmed RTC sync
		// use semaphore or signal
		rtc_synced.wait(); // wait for semaphore availability
		// Thread::signal_wait(0x01, osWaitForever); // wait for signal to be set from the rtc sync thread
		time_t ts_s = time(NULL);// get the timestamp for the measuremenet
		rtc_synced.release(); // semaphore release

		// Converts value in the 0V-3.3V range and send to print queue
		//evqueue->call(print_voltage, ana_reading[0], 0 ,ts_s);
		vref = (ana_reading[1]/ana_reading[0])*2048;
		evqueue->call(print_voltage, vref, 1 ,ts_s);

		Thread::wait(2000);
	}
}


static void TIM3_Config(void) {
	/* TIM3 with interrupt configuration */

	// 1:  enable clock source
	__HAL_RCC_TIM3_CLK_ENABLE();

	// 2: get the clock source frequency and calculate prescaler and period settings based on that
	RCC_ClkInitTypeDef clkinitstruct = {0};         /* Temporary variable to retrieve RCC clock configuration */
	uint32_t tmpdummy;                              /* Temporary variable to retrieve RCC clock configuration */
	uint32_t timer_clock_source_freq = 0;           /* Timer clock source frequency */
	uint32_t timer_prescaler_timebase_freq_min = 0; /* Time base prescaler to have timebase aligned on minimum frequency possible */

	/* Configuration of timer as time base */

	// Retrieve timer clock source frequency
	HAL_RCC_GetClockConfig(&clkinitstruct, &tmpdummy);

	/* If APB1 prescaler is different of 1, timers have a factor x2 on their
	clock source.                                                            */
	if (clkinitstruct.APB1CLKDivider == RCC_HCLK_DIV1) {
		timer_clock_source_freq = HAL_RCC_GetPCLK1Freq();
	} else {
		timer_clock_source_freq = HAL_RCC_GetPCLK1Freq() *2;
	}

	/* Computation of timer prescaler */
	/* (computation for timer 16 bits, additional +1 to round the prescaler up) */
	timer_prescaler_timebase_freq_min = (timer_clock_source_freq / ( (0xFFFF-1) * TIMER_FREQUENCY_RANGE_MIN_HZ)) +1;

	// 3: configure timer parameters
	mTimHandle.Instance = TIM3; 	// Set timer instance
	mTimHandle.Init.Period           = ((timer_clock_source_freq / (timer_prescaler_timebase_freq_min * TIMER_FREQUENCY_HZ)) - 1);
	mTimHandle.Init.Prescaler        = (timer_prescaler_timebase_freq_min - 1);
	mTimHandle.Init.ClockDivision    = TIM_CLOCKDIVISION_DIV1;
	mTimHandle.Init.CounterMode      = TIM_COUNTERMODE_UP;
	mTimHandle.Init.RepetitionCounter= 0x0;

	// 4: apply settings
	if (HAL_TIM_Base_Init(&mTimHandle) != HAL_OK) {
		TIM3_Error_Handler();
	}

	// 5: enable interrupt in the NVIC interrupt controller
	//NVIC_SetVector(TIM3_IRQn,(uint32_t)&HAL_TIM_IRQHandler(&mTimHandle)); // runtime vector redirect
	HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0); //highest priority
	HAL_NVIC_EnableIRQ(TIM3_IRQn);

	// 6: start timer in interrupt mode
	if (HAL_TIM_Base_Start_IT(&mTimHandle) != HAL_OK ) {
		TIM3_Error_Handler();
	}

	/*J: more for ADC trigger later on
	// Timer TRGO selection
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

	if (HAL_TIMEx_MasterConfigSynchronization(&TimHandle, &sMasterConfig) != HAL_OK)
	{
		// Timer TRGO selection Error
		TIM3_Error_Handler();
	} */
}


// halt code execution of timer interrupt and go here without interrupting other threads
static void TIM3_Error_Handler(void)
{
  while(1) {
   	Thread::wait(500);
  }
}


void print_voltage(float voltage, uint8_t analog_pin, time_t timestamp) {
	printf("ts : %ld , V(A%u) = %.0f mV \r\n", timestamp, analog_pin,  voltage);
	// printf("date/time: %s\r\n", ctime(&timestamp));
}

void print_voltage_int(uint16_t voltage, uint8_t analog_pin, time_t timestamp) {
	printf("ts : %ld , V(A%u) = %d \r\n", timestamp, analog_pin,  voltage);
}

void print_wifi_status(bool wifi_status) {
	if (wifi_status) {
		printf("> Connected.\r\n");
		printf("  MAC: %s\r\n", wifi.get_mac_address());
		printf("  IP: %s\r\n", wifi.get_ip_address());
		printf("  Netmask: %s\r\n", wifi.get_netmask());
		printf("  Gateway: %s\r\n", wifi.get_gateway());
		printf("  RSSI: %d\r\n\r\n", wifi.get_rssi());
	} else {
		printf("> Cannot connect to AP.\r\n");
	}
}

bool get_unix_timestamp_j(WiFiInterface *wifi_stack, time_t* timestamp) {

	bool ret = false;
	TCPSocket socket;
	nsapi_error_t response;
	const char timeServer_URL[] = "www.convert-unix-time.com";
	const char timeServer_endpoint[] = "/api?timestamp=now&timezone=New_York";
	uint16_t timeServer_port = 80;
	char http_buffer[512];

	printf("> Sending HTTP request to %s...\r\n", timeServer_URL);
	socket.open(wifi_stack);
	response = socket.connect(timeServer_URL, timeServer_port);
	if(0 != response) {
		printf("> Cannot open TCP socket : %d\r\n", response);
		socket.close();
	} else {
		// Send a simple http request
		sprintf(http_buffer, "GET %s HTTP/1.1\r\nHost: %s \r\n", timeServer_endpoint, timeServer_URL);
		strcat(http_buffer, "Connection: Close\r\n\r\n");
		nsapi_size_t req_len = strlen(http_buffer);
		response = 0;
		while(req_len) // using a while is useful for long request, but it is optional here
		{
			printf("> sending %d bytes... \r\n", req_len);
			response = socket.send(http_buffer+response, req_len);
			if (response < 0) {
				printf("> Error sending data: %d\r\n", response);
				socket.close();
				break;
			} else {
				req_len -= response;
				// Check if entire message was sent or not
				printf("> sent %d bytes. \r\n", response);
			}
		}

		// Receive a simple http response and print out the response line
		response = socket.recv(http_buffer, sizeof(http_buffer));
		if (response < 0) {
			printf("> Error receiving data: %d\r\n", response);
		} else {
			printf("> Received %d bytes.\r\n ", response);
			//printf("%s \r\n", http_buffer);
			// extract the body (json payload) from the response
			char *j_start = strchr(http_buffer, '{'); // get to start of json payload
			char *j_end = strchr(http_buffer, '}');
			char resp_payload[512];
			memset(resp_payload, '\0', sizeof(resp_payload));
			strncpy(resp_payload, j_start, j_end - j_start + 1);
			if (strlen(resp_payload) > 0) {
				printf(resp_payload);
				printf("\r\n\r\n");
				ret = true;
				/*
				picojson::value json_val;
				char* foo = resp_payload;
				string err = picojson::parse(json_val, foo, foo + strlen(resp_payload));
				if(err.empty()) {
					uint32_t unix_time = (unsigned)json_val.get("timestamp").get<double>();
					printf("> Unix timestamp = %d \r\n", unix_time);
					set_time((time_t)unix_time);
					rtc_synced.release();
					ret = true;
				} else {
					printf("> ERROR: json parsing\r\n");
				}
				*/
			} else {
				printf("> ERROR: response parsing\r\n");
			}
		}
		// Close the socket to return its memory and bring down the network interface
		socket.close();
	}
	return ret;
}




