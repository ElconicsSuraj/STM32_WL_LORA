#define ACTIVATION_MODE     		OTAA
#define CLASS						CLASS_C
#define SPREADING_FACTOR    		12
#define ADAPTIVE_DR         		false
#define CONFIRMED           		false
#define APP_PORT                	2

#define SEND_BY_PUSH_BUTTON 		false
#define FRAME_DELAY         		15000
#define PAYLOAD_1234				true
#define PAYLOAD_TEMPERATURE    		false
#define PAYLOAD_HUMIDITY   			false
#define CAYENNE_LPP_         		false
#define LOW_POWER           		false


#define devEUI_						{ 0x90,0x54,0x35,0x67,0x89, 0x03, 0x20, 0x98} //device 4
//#define devEUI_						{0x23, 0x07, 0x20, 0x25, 0x00, 0x00, 0x00, 0x03} // device 3

// Configuration for ABP Activation Mode
#define devAddr_ 					( uint32_t )0x00000000
#define nwkSKey_ 					00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00
#define appSKey_ 					00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00


// Configuration for OTAA Activation Mode
#define appKey_						 0F,F4,DA,81,4C,3F,00,D1,11,B8,89,12,E1,17,0C,6E // device 4
//#define appKey_						EE,A6,9A,88,8F,39,A7,49,BD,B8,44,B8,3B,BC,05,B0// device 3
//#define appKey                     0x0F, 0xF4, 0xDA, 0x81, 0x4C, 0x3F, 0x00, 0xD1, 0x11, 0xB8, 0x89, 0x12, 0xE1, 0x17, 0x0C, 0x6E
#define appEUI_				    {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}  // dev 4
//#define appEUI_						{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01} // device 3
