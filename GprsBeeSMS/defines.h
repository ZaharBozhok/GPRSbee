#pragma once
//pins
#define TEMPERATURE_SENSOR_PIN	PD4
#define GPRS_RST				A2
//#define GPRS_PWR				A1
#define SIM800_TX_PIN			10
#define SIM800_RX_PIN			9
#define WEIGHTS_DT_PIN			18
#define WEIGHTS_SCK_PIN			19
#define INT_PIN_0				PD2
#define INT_PIN_1				PD3
#define RGB_PIN_R				6
#define RGB_PIN_G				7
#define RGB_PIN_B				8
#define SLR_PWR					5
#define LIGHT_PIN				A3
#define PWR_PIN					A0

//code
#define MAX_SMS_COMMANDS		    3
//Attention! Raising of max_sms_commands will cause long delay reading stack
#define MAX_NUMS_SIZE			      5
#define DEF_ALRM_STATE			    TRUE
#define NUMBER_SIZE				      13
#define SMS_SIZE				        30
#define URL						          "http://gprsbee.azurewebsites.net/main.php"
#define APN						          "www.ab.kyivastar.net"
#define FRONT_0					        FALLING
#define FRONT_1					        FALLING
#define OWNER_NUMBER			      "+380503213610"
#define START_CONFIGURING_MODE	FALSE
#define START_TIMER_CHAR		    'h'
#define START_TIMER_VALUE		    2
#define CONF_MULTIPLIER         12
#define CONF_AWAIT_TIME_MIN     30
#define CONF_AWAIT_TIME_TICK CONF_MULTIPLIER * CONF_AWAIT_TIME_MIN
#define AREF                    4.33
//#define DEBUG
