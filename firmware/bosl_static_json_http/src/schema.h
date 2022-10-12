#include "Arduino.h"

// DO NOT DECLARE CONST, will cause memory-copy
char jsonSchema[] = R"EOF({"device_name":"UNKNOWN","imei":"123123123","battery_mv":4200,"bootup_timestamp":null,"chmln_top_raw_a":null,"chmln_top_raw_b":null,"chmln_top_raw_avg":null,"chmln_top_resistance":null,"chmln_bot_raw_a":null,"chmln_bot_raw_b":null,"chmln_bot_raw_avg":null,"chmln_bot_resistance":null,"ds18b20_bot_temperature":null,"ds18b20_top_temperature":null,"smt100_top_vwc":null,"smt100_top_temperature":null})EOF";