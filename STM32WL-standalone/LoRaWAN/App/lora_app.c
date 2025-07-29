
#include "platform.h"
#include "Region.h" /* Needed for LORAWAN_DEFAULT_DATA_RATE */
#include "sys_app.h"
#include "lora_app.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "lora_app_version.h"
#include "lorawan_version.h"
#include "subghz_phy_version.h"
#include "lora_info.h"
#include "LmHandler.h"
#include "stm32_lpm.h"
#include "adc_if.h"
#include "sys_conf.h"
//#include "CayenneLpp.h"
//#include "sys_sensors.h"
#include "radio.h"
#include "send_raw_lora.h"
#include "stdlib.h"

#define LINE_SIZE 34
#include  "General_Setup.h"
uint8_t simuTemperature(void);
uint8_t simuHumidity(void);

static void byteReception(uint8_t *PData, uint16_t Size, uint8_t Error);
#define RX_BUFF_SIZE 250

static uint8_t rxBuff[RX_BUFF_SIZE];
uint8_t isRxConfirmed;
uint32_t LoRaMode = 0;
uint8_t size_txBUFFER = 0;
uint8_t txBUFFER[100];
uint8_t isTriggered = 0;



static uint8_t rx_byte;
static uint32_t rx_counter = 0;
static uint8_t downlink_data_buffer[RX_BUFF_SIZE];
static uint8_t downlink_processing_buffer[RX_BUFF_SIZE];
uint8_t downlink_data_length = 0;

typedef enum TxEventType_e
{
	TX_ON_TIMER,
	TX_ON_EVENT
} TxEventType_t;

static void SendTxData(void);
static void OnTxTimerEvent(void *context);
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams);
static void OnTxData(LmHandlerTxParams_t *params);
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);
static void OnMacProcessNotify(void);

static void OnTxTimerLedEvent(void *context);
static void OnRxTimerLedEvent(void *context);
static void OnJoinTimerLedEvent(void *context);

static void MX_BP_IT_Init(void);

static void while_loop(void);

static ActivationType_t ActivationType = LORAWAN_DEFAULT_ACTIVATION_TYPE;

static LmHandlerCallbacks_t LmHandlerCallbacks =
{
		.GetBatteryLevel =           GetBatteryLevel,
		.GetTemperature =            GetTemperatureLevel,
		.GetUniqueId =               GetUniqueId,
		.GetDevAddr =                GetDevAddr,
		.OnMacProcess =              OnMacProcessNotify,
		.OnJoinRequest =             OnJoinRequest,
		.OnTxData =                  OnTxData,
		.OnRxData =                  OnRxData
};

static LmHandlerParams_t LmHandlerParams =
{
		.ActiveRegion =             ACTIVE_REGION,
		.DefaultClass =             LORAWAN_DEFAULT_CLASS,
		.AdrEnable =                LORAWAN_ADR_STATE,
		.TxDatarate =               LORAWAN_DEFAULT_DATA_RATE,
		.PingPeriodicity =          LORAWAN_DEFAULT_PING_SLOT_PERIODICITY
};

//static TxEventType_t EventType = ADMIN_TX_TYPE;
static UTIL_TIMER_Object_t TxTimer;
static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];
static LmHandlerAppData_t AppData = { 0, 0, AppDataBuffer };
static uint8_t AppLedStateOn = RESET;
static UTIL_TIMER_Object_t TxLedTimer;
static UTIL_TIMER_Object_t RxLedTimer;
static UTIL_TIMER_Object_t JoinLedTimer;

void LoRaWAN_Init(void)
{
	MX_BP_IT_Init();

	// Starts the RX USART2 process by interrupt
	UTIL_ADV_TRACE_StartRxProcess(byteReception);

	APP_LOG_COLOR(CLEAR_SCREEN);
	APP_LOG_COLOR(RESET_COLOR);
	APP_LOG(0, 1, " \r\n");
	APP_LOG(0, 1, "########################################\r\n");
	APP_LOG(0, 1, "###### ANTZ LORAWAN CONTROL CARD ########\r\n");
	APP_LOG(0, 1, "###### Suraj Kumar ####\r\n");
	APP_LOG(0, 1, " \r\n");




	BSP_PB_Init(BUTTON_SW1, BUTTON_MODE_EXTI);		// BUTTON_SW1 = PA0, IRQ number = EXTI0_IRQn
	BSP_PB_Init(BUTTON_SW2, BUTTON_MODE_GPIO);

	/****** Raw LoRa Packet Application *************/
	if ( BSP_PB_GetState(BUTTON_SW2) == 0 || RAW_LORA_APP == true) {

		LoRaMode = 1;
		APP_LOG_COLOR(BLUE);
		APP_LOG(0, 1, "Raw LoRa Packet Application\r\n\r\n");

		APP_LOG_COLOR(WHITE);
		APP_LOG(0, 1, "Type the following command to send a Raw LoRa Packet\r\n");
		APP_LOG(0, 1, "> Command format : LORA=Frequency:Power:SF:Payload\r\n");
		APP_LOG(0, 1, "> Example :        LORA=868100000:14:7:01020304 \r\n\r\n");


	}

	/***** LoRaWAN Standalone Application  ***********/
	else{
		APP_LOG(0, 1, "> Activation mode         %s",(ACTIVATION_MODE == ABP) ? "ABP \r\n" : "OTAA \r\n");

		if(CLASS == CLASS_A){
			APP_LOG(0, 1, "> Class                   A\r\n");

		}
		else if(CLASS == CLASS_B){
			APP_LOG(0, 1, "> Class                   B\r\n");

		}
		else if(CLASS == CLASS_C){
			APP_LOG(0, 1, "> Class                   C\r\n");

		}
		if(SEND_BY_PUSH_BUTTON == true){
			APP_LOG(0, 1, "> Send frame              On push button event \r\n");

		}
		else{
			APP_LOG(0, 1, "> Send frame every        %d ms\r\n", ADMIN_TxDutyCycleTime);

		}
		APP_LOG(0, 1, "> Spreading Factor        %d \r\n", SPREADING_FACTOR);
		APP_LOG(0, 1, "> Adaptive Data Rate      %s", (ADAPTIVE_DR == true) ? "ON \r\n" : "OFF \r\n");
		APP_LOG(0, 1, "> Uplink Frame            %s",(CONFIRMED == true) ? "Confirmed\r\n" : "Unconfirmed\r\n");
		APP_LOG(0, 1, "> App Port number         %d \r\n", (!WATTECO_TEMPO)? APP_PORT: WATTECO_TEMPO_PORT);



		if(CAYENNE_LPP && SENSOR_ENABLED){
			APP_LOG(0, 1, "> Payload content:        CayenneLPP, sensors\r\n");

		}
		else if(CAYENNE_LPP && !SENSOR_ENABLED){
			APP_LOG(0, 1, "> Payload content:        CayenneLPP, simulated values\r\n");

		}
		else if(PAYLOAD_TEMPERATURE && !PAYLOAD_HUMIDITY && SENSOR_ENABLED){
			APP_LOG(0, 1, "> Payload content         1-byte temperature\r\n");

		}
		else if(PAYLOAD_TEMPERATURE && PAYLOAD_HUMIDITY && SENSOR_ENABLED){
			APP_LOG(0, 1, "> Payload content         1-byte temperature + humidity\r\n");

		}
		else if(!PAYLOAD_TEMPERATURE && PAYLOAD_HUMIDITY && SENSOR_ENABLED){
			APP_LOG(0, 1, "> Payload content         1-byte humidity\r\n");

		}
		else if(PAYLOAD_TEMPERATURE && !PAYLOAD_HUMIDITY && !SENSOR_ENABLED){
			APP_LOG(0, 1, "> Payload content         1-byte simulated temperature\r\n");

		}
		else if(PAYLOAD_TEMPERATURE && PAYLOAD_HUMIDITY && !SENSOR_ENABLED){
			APP_LOG(0, 1, "> Payload content         1-byte simulated temperature + humidity\r\n");

		}
		else if(!PAYLOAD_TEMPERATURE && PAYLOAD_HUMIDITY && !SENSOR_ENABLED){
			APP_LOG(0, 1, "> Payload content         1-byte simulated humidity\r\n");

		}
		else if(PAYLOAD_1234 == true){
			APP_LOG(0, 1, "> Payload content         0x01 0x02 0x03 0x04\r\n");

		}
		else if(CAYENNE_LPP == true){
			APP_LOG(0, 1, "> Payload content         CayenneLPP, sensors\r\n");

		}
		else if(USMB_VALVE == true){
			APP_LOG(0, 1, "> Payload content         1 byte setpoint + 1 byte temperature\r\n");

		}
		else if(ATIM_THAQ == true){
			APP_LOG(0, 1, "> Payload content         17 bytes ATIM_THAQ simulated payload\r\n");

		}
		else if(WATTECO_TEMPO == true){
			APP_LOG(0, 1, "> Payload content         32 bytes WATTECO_TEMP'O fixed simulated payload\r\n");

		}
		else if(TCT_EGREEN == true){
			APP_LOG(0, 1, "> Payload content         10 bytes TCT_EGREEN simulated payload\r\n");

		}

		APP_LOG(0, 1, "\r\n");

		APP_LOG_COLOR(BLUE);



		UTIL_TIMER_Create(&TxLedTimer, 0xFFFFFFFFU, UTIL_TIMER_ONESHOT, OnTxTimerLedEvent, NULL);
		UTIL_TIMER_Create(&RxLedTimer, 0xFFFFFFFFU, UTIL_TIMER_ONESHOT, OnRxTimerLedEvent, NULL);
		UTIL_TIMER_Create(&JoinLedTimer, 0xFFFFFFFFU, UTIL_TIMER_PERIODIC, OnJoinTimerLedEvent, NULL);
		UTIL_TIMER_SetPeriod(&TxLedTimer, 500);
		UTIL_TIMER_SetPeriod(&RxLedTimer, 500);
		UTIL_TIMER_SetPeriod(&JoinLedTimer, 1000);

		UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LmHandlerProcess), UTIL_SEQ_RFU, LmHandlerProcess);
		UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), UTIL_SEQ_RFU, SendTxData);
		//UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_DisplayOnLCD), UTIL_SEQ_RFU, lcd_print_buf);  // 251
		UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_DisplayOnLCD), UTIL_SEQ_RFU, SendTxData);

		LoraInfo_Init();

		/* Initialize all callbacks */
		LmHandlerInit(&LmHandlerCallbacks);

		/* Print LoRaWAN information : DevEUI & Devaddr when ABP - DevEUI & AppEUI-JoinEUI for OTAA */
		/* Print Session Keys for ABP - Print Root Key for OTAA :LmHandlerConfigure() > LoRaMacInitialization() > SecureElementInit() > PrintKey() */
		LmHandlerConfigure(&LmHandlerParams);

		/* Let all print out terminated. Otherwise logs are affected.*/
		HAL_Delay(500);

		/* Join Red LED starts blinking */
		UTIL_TIMER_Start(&JoinLedTimer);

		/* Join procedure for OTAA */
		/* First try to Join Network. Next time the Device tries to send data (LmHandlerSend), it will check the Join.
		 * If the first Join was NOT successful, it sends another Join.
		 */
		LmHandlerJoin(ActivationType);
	}
}




static void byteReception(uint8_t *PData, uint16_t Size, uint8_t Error)
{
	rx_byte = *PData;
	printf("I am here at 1\n ");

	if (rx_counter >= (RX_BUFF_SIZE - 4)) {
		rx_counter = 0;
	}

	if (rx_byte == 0) {
		// Ignore NULL byte
	}
	else if (rx_byte == '\n') {
		downlink_data_buffer[rx_counter++] = '\0';

		if (rx_counter > 1) {
			memset(downlink_processing_buffer, 0, sizeof(downlink_processing_buffer));
			memcpy(downlink_processing_buffer, downlink_data_buffer, rx_counter);

			APP_LOG(0, 1, "Received Data: %s\n", downlink_processing_buffer);

			rx_counter = 0;

			UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);
		} else {
			rx_counter = 0;
		}
	}
	else {
		downlink_data_buffer[rx_counter++] = rx_byte;
	}
}






void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin)
	{
	case  BUTTON_SW1_PIN:
		UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);
		break;

	default:
		break;
	}
}

void EXTI9_5_IRQHandler(void)
{
	if ((EXTI->PR1 & EXTI_PR1_PIF8) == EXTI_PR1_PIF8) {
		// GFX01M2 right button pressed
		// Pin 8 is IT source
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);

	}
	else if ((EXTI->PR1 & EXTI_PR1_PIF9) == EXTI_PR1_PIF9) {
		// GFX01M2 center button pressed
		// Pin 9 is IT source
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
	}
	else __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8 | GPIO_PIN_9);

	UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_DisplayOnLCD), CFG_SEQ_Prio_LCD);
}

void EXTI4_IRQHandler(void)
{
	// GFX01M2 left button pressed. Right button application should also be handled here
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}







static void SendTxData(void)
{
	UTIL_TIMER_Time_t nextTxIn = 0;
	uint32_t i = 0;
	uint8_t channel = 0;



	if (WATTECO_TEMPO) AppData.Port = WATTECO_TEMPO_PORT;
	else AppData.Port = APP_PORT;

	if(PAYLOAD_1234){
		AppData.Buffer[i++] = downlink_processing_buffer[0];
		AppData.Buffer[i++] = downlink_processing_buffer[1];
		AppData.Buffer[i++] = downlink_processing_buffer[2];
		AppData.Buffer[i++] = downlink_processing_buffer[3];
		AppData.Buffer[i++] = downlink_processing_buffer[4];
		AppData.Buffer[i++] = downlink_processing_buffer[5];
		AppData.Buffer[i++] = downlink_processing_buffer[6];
		AppData.Buffer[i++] = downlink_processing_buffer[7];
		AppData.Buffer[i++] = downlink_processing_buffer[8];
		AppData.Buffer[i++] = downlink_processing_buffer[9];
		AppData.BufferSize = i;



	}



		AppData.BufferSize = i;


	LoRaMacTxInfo_t txInfo;

	if (LORAMAC_HANDLER_SUCCESS == LmHandlerSend(&AppData, LORAWAN_DEFAULT_CONFIRMED_MSG_STATE, &nextTxIn, false))
	{

	}
	else if (nextTxIn > 0)
	{
		APP_LOG(TS_ON, VLEVEL_L, "Next Tx in  : ~%d second(s)\r\n", (nextTxIn / 1000));
	}
}





static void OnTxData(LmHandlerTxParams_t *params)
{
	if ((params != NULL))
	{
		/* Process Tx event only if it is a mcps response to prevent some internal events (mlme) */
		if (params->IsMcpsConfirm != 0)
		{
			if(params->AppData.Port != 0){
				BSP_LED_On(LED_GREEN) ;
				UTIL_TIMER_Start(&TxLedTimer);

				// Print Timestamp
				char timestamp[20];
				uint16_t * size;
				TimestampNow((uint8_t *) timestamp, size);
				char stimestamp[32];
				strcpy(stimestamp, "_");
				strcat(stimestamp, timestamp+2);
#ifdef DISPLAY_NB_LINES
				strcat(stimestamp, "______________");
#else
				strcat(stimestamp, "________________");
#endif // DISPLAY_NB_LINE


				APP_LOG(0, 1, "- Payload     ");


				if(AppData.BufferSize>0)
				{
					if(USMB_VALVE){
						APP_LOG(0, 1, "Setpoint: %.1f °C | Temperature: %.1f °C", ((float) txBUFFER[0])/2, ((float) txBUFFER[1])/2);

					}
					else if(ATIM_THAQ){
						APP_LOG(0, 1, "Temperature: %.2f °C", ((float) (txBUFFER[6]<<8) + (float) txBUFFER[7])/100.0);

					}
					else if(WATTECO_TEMPO){ // Simply print the buffer
						for(int i=0;i<size_txBUFFER;i++){
							APP_LOG(0, 1, "%02X ", txBUFFER[i]);
						}

					}
					else if(TCT_EGREEN){
						APP_LOG(0, 1, "Currant: %.2f A | Voltage: %03u mV", ((float) (txBUFFER[5]<<8) + (float) txBUFFER[6])/100.0,
																			(txBUFFER[8]<<8) + txBUFFER[9]);

					}
					else{
						for(int i=0;i<size_txBUFFER;i++){
							APP_LOG(0, 1, "%02X ", txBUFFER[i]);
						}
						APP_LOG(0, 1, "(hex)  |  ");

						for(int i=0;i<size_txBUFFER;i++){
							APP_LOG(0, 1, "%03u ", txBUFFER[i]);
						}
						APP_LOG(0, 1, "(dec)");

					}
				}

				APP_LOG(TS_OFF, VLEVEL_L, "\r\n");
				APP_LOG(TS_OFF, VLEVEL_L, "- Port        %d \r\n",params->AppData.Port);
				APP_LOG(TS_OFF, VLEVEL_L, "- Fcnt        %d \r\n",params->UplinkCounter);
				APP_LOG(TS_OFF, VLEVEL_L, "- Data Rate   %d",params->Datarate);


				switch(params->Datarate)
				{
					case 5 : APP_LOG(TS_OFF, VLEVEL_L, " (SF7)\r\n");break;
					case 4 : APP_LOG(TS_OFF, VLEVEL_L, " (SF8)\r\n");break;
					case 3 : APP_LOG(TS_OFF, VLEVEL_L, " (SF9)\r\n");break;
					case 2 : APP_LOG(TS_OFF, VLEVEL_L, " (SF10)\r\n");break;
					case 1 : APP_LOG(TS_OFF, VLEVEL_L, " (SF11)\r\n");break;
					case 0 : APP_LOG(TS_OFF, VLEVEL_L, " (SF12)\r\n");break;
				}

			}
			else{
				APP_LOG(TS_OFF, VLEVEL_L, "- Fcnt        %d \r\n",params->UplinkCounter);
			}

		}
	}
}


static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
	static const char *slotStrings[] = { "1", "2", "C", "C Multicast", "B Ping-Slot", "B Multicast Ping-Slot" };
	char is_join = 1;

	if ((appData != NULL) || (params != NULL))
	{
		if( appData->Port == 0){
			// if port==0, it is join accept or MAC command
			//APP_LOG(TS_OFF, VLEVEL_L, "MAC Command RECEIVED\r\n");
			is_join = 0;
		}
		else{
			BSP_LED_On(LED_BLUE) ;
			UTIL_TIMER_Start(&RxLedTimer);
			APP_LOG_COLOR(BOLDBLUE);
//			lcd_printf(LCD_BLUE, "________________________________");
			if (isRxConfirmed == 1){
				APP_LOG(TS_ON, VLEVEL_L, " Receiving Confirmed Data Down.\r\n");
//				lcd_printf(LCD_BLUE, "Receiving Confirmed Data Down.");
			}
			else{
				APP_LOG(TS_ON, VLEVEL_L, " Receiving Unconfirmed Data Down.\r\n");
//				lcd_printf(LCD_BLUE, "Receiving Unconf Data Down");
			}

			APP_LOG_COLOR(BLUE);
			APP_LOG(TS_OFF, VLEVEL_L, "- Payload     ");

			if(appData->BufferSize>0)
			{
				char payload[LINE_SIZE] = "- Payload   ";
				char val[16];
				for(int i=0 ; i<appData->BufferSize ; i++){
					APP_LOG(0, 1, "%02X ", appData->Buffer[i]);
					itoa(appData->Buffer[i], val, 16);
					strcat(payload, val);
					strcat(payload, " ");
				}
				APP_LOG(0, 1, "(hex) ");
				strcat(payload, "(hex)");
//				lcd_printf(LCD_BLUE, payload);
			}

			APP_LOG(TS_OFF, VLEVEL_L, "\r\n");
			APP_LOG(TS_OFF, VLEVEL_L, "- Port        %d \r\n",appData->Port);
			APP_LOG(TS_OFF, VLEVEL_L, "- Slot        RX%s \r\n",slotStrings[params->RxSlot]);
			APP_LOG(TS_OFF, VLEVEL_L, "- Data Rate   %d \r\n",params->Datarate);
			APP_LOG(TS_OFF, VLEVEL_L, "- RSSI        %d dBm\r\n",params->Rssi);
			APP_LOG(TS_OFF, VLEVEL_L, "- SNR         %d dB\r\n",params->Snr);
			APP_LOG_COLOR(RESET_COLOR);


		}

		switch (appData->Port)
		{
			case LORAWAN_SWITCH_CLASS_PORT:
				if (appData->BufferSize == 1)
				{
					APP_LOG_COLOR(GREEN);
					switch (appData->Buffer[0])
					{
						case 0: LmHandlerRequestClass(CLASS_A); APP_LOG(TS_OFF, VLEVEL_L, "Switch to class A\r\n");
						//lcd_printf(LCD_DGREEN, "Switch to class A");
						break;
						case 1: LmHandlerRequestClass(CLASS_B); APP_LOG(TS_OFF, VLEVEL_L, "Switch to class B\r\n");
						//lcd_printf(LCD_DGREEN, "Switch to class B");
						break;
						case 2: LmHandlerRequestClass(CLASS_C); APP_LOG(TS_OFF, VLEVEL_L, "Switch to class C\r\n");
						//lcd_printf(LCD_DGREEN, "Switch to class C");
						break;
						default:
						break;
					}
					APP_LOG_COLOR(RESET_COLOR);
				}
				break;

			case LORAWAN_USER_APP_PORT:
				if (appData->BufferSize == 1)
				{
					APP_LOG_COLOR(GREEN);
					AppLedStateOn = appData->Buffer[0] & 0x01;
					if (AppLedStateOn == RESET)
					{
						APP_LOG(TS_OFF, VLEVEL_L, "LED 3 (RED) goes OFF\r\n");

						BSP_LED_Off(LED_RED) ;
					}
					else
					{
						APP_LOG(TS_OFF, VLEVEL_L, "LED 3 (RED) goes ON\r\n");

						BSP_LED_On(LED_RED) ;
					}
					APP_LOG_COLOR(RESET_COLOR);
				}
				break;

			case VALVE_APP_PORT:
				if (appData->BufferSize == 1)
				{
					APP_LOG_COLOR(GREEN);

					APP_LOG_COLOR(RESET_COLOR);

				}
				break;

			default:  break;
		}

		if (is_join != 0) {

		}
	}
}
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams)
{
	if (joinParams != NULL)
	{
		if (joinParams->Status == LORAMAC_HANDLER_SUCCESS)
		{
			UTIL_TIMER_Stop(&JoinLedTimer);
			BSP_LED_Off(LED_RED) ;

			APP_LOG_COLOR(GREEN);
			if (joinParams->Mode == ACTIVATION_TYPE_ABP)
			{
				APP_LOG(TS_OFF, VLEVEL_L, "\r\n> ABP Activation mode\r\n");

			}
			else
			{
				APP_LOG(TS_OFF, VLEVEL_L, "\r\n> JOINED = OTAA!\r\n");

			}


			if(SEND_BY_PUSH_BUTTON == true){
				APP_LOG(0, 1, "> Packets will be sent every %d ms OR on a Push Button event (B1) \r\n", ADMIN_TxDutyCycleTime);

			}
			else{
				APP_LOG(0, 1, "> Packets will be sent every %d ms OR on a Push Button event (B1) \r\n", ADMIN_TxDutyCycleTime);

			}

			APP_LOG_COLOR(RESET_COLOR);

			/* Send every time button is pushed */
			if (SEND_BY_PUSH_BUTTON == false)
			{
				UTIL_TIMER_Create(&TxTimer,  0xFFFFFFFFU, UTIL_TIMER_ONESHOT, OnTxTimerEvent, NULL);
				UTIL_TIMER_SetPeriod(&TxTimer,  APP_TX_DUTYCYCLE);
				UTIL_TIMER_Start(&TxTimer);
			}



			// Send a first frame just after the Join (When using timer event to send packets)
			if(SEND_BY_PUSH_BUTTON == false){
				SendTxData();
			}
		}
		else
		{
			APP_LOG_COLOR(RED);
			APP_LOG(TS_OFF, VLEVEL_L, "\r\n> JOIN FAILED...\r\n");



			LmHandlerJoin(ActivationType);
			APP_LOG_COLOR(RESET_COLOR);
			APP_LOG(0, 1, " \r\n");
		}
	}
}

static void OnMacProcessNotify(void)
{

	UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LmHandlerProcess), CFG_SEQ_Prio_0);

}


static void OnTxTimerEvent(void *context)
{
	UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);

	/*Wait for next tx slot*/
	UTIL_TIMER_Start(&TxTimer);
}

static void OnTxTimerLedEvent(void *context)
{
	BSP_LED_Off(LED_GREEN) ;
}

static void OnRxTimerLedEvent(void *context)
{
	BSP_LED_Off(LED_BLUE) ;
}

static void OnJoinTimerLedEvent(void *context)
{
	BSP_LED_Toggle(LED_RED) ;
}

/**
  * @brief Interruption button Initialization Function
  * @param None
  * @retval None
  */
static void MX_BP_IT_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	 /*Configure GPIO pin: PA9, Center button */
	 GPIO_InitStruct.Pin = GPIO_PIN_9;
	 GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	 GPIO_InitStruct.Pull = GPIO_NOPULL;
	 HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	 /*Configure GPIO pin: PA4, Left button */
	 GPIO_InitStruct.Pin = GPIO_PIN_4;
	 GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	 GPIO_InitStruct.Pull = GPIO_NOPULL;
	 HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	 /*Configure GPIO pin: PA8, Down button */
	 GPIO_InitStruct.Pin = GPIO_PIN_8;
	 GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	 GPIO_InitStruct.Pull = GPIO_NOPULL;
	 HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	 /* EXTI interrupt init*/
	 HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 1);
	 HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	 HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
	 HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}
