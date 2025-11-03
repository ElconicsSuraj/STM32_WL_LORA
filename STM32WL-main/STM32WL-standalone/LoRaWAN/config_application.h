#define ACTIVATION_MODE     		OTAA
#define CLASS						CLASS_C
#define SPREADING_FACTOR    		12
#define ADAPTIVE_DR         		false
#define CONFIRMED           		false
#define APP_PORT                	2

#define SEND_BY_PUSH_BUTTON 		false
#define FRAME_DELAY         		12000
#define PAYLOAD_1234				true
#define PAYLOAD_TEMPERATURE    		false
#define PAYLOAD_HUMIDITY   			false
#define CAYENNE_LPP_         		false
#define LOW_POWER           		false


#define devEUI_						{0x70,0x91,0x77,0x99,0x31, 0x82, 0x53, 0x01}
//#define devEUI_						{ 0xE4, 0x56, 0x78, 0x90, 0x87, 0xDF, 0xBC, 0xF9}// E4,56,78,90,87,DFBCF9
                                   //  90,54,35,67,89,03,20,98
// Configuration for ABP Activation Mode
#define devAddr_ 					( uint32_t )0x00000000
#define nwkSKey_ 					00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00
#define appSKey_ 					00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00


// Configuration for OTAA Activation Mode
#define appKey_						 0C,24,6F,23,BF,B6,DA,D9,19,85,09,F1,DA,19,44,B4
//#define appKey                     0x0F, 0xF4, 0xDA, 0x81, 0x4C, 0x3F, 0x00, 0xD1, 0x11, 0xB8, 0x89, 0x12, 0xE1, 0x17, 0x0C, 0x6E
//#define appEUI_				    {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}  dev 4
#define appEUI_						{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}
