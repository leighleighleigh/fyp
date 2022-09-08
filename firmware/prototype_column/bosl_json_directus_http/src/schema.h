#include <Arduino.h>

// const char *column_data_schema = "{\"bosl_name\":\"%s\",\"bosl_imei\":\"%s\",\"bosl_battery_mv\":%d,\"bosl_bootup_timestamp\":%s,\"chmln_top_raw_a\":%d,\"chmln_top_raw_b\":%d,\"chmln_top_raw_avg\":%d,\"chmln_top_resistance\":%d,\"chmln_bot_raw_a\":%d,\"chmln_bot_raw_b\":%d,\"chmln_bot_raw_avg\":%s,\"chmln_bot_resistance\":%d,\"ds18b20_bot_temperature\":%s,\"ds18b20_top_temperature\":%s,\"smt100_top_vwc\":%s,\"smt100_top_temperature\":%s,\"smt100_bot_vwc\":%d,\"smt100_bot_temperature\":%d}";
const char *column_data_schema = "{\"bosl_name\":\"%s\",\"bosl_imei\":\"%s\",\"bosl_battery_mv\":%d}";

/*
{
  "id": 4,
  "date_created": "2022-09-02T03:31:00.791Z",
  "bosl_name": "BW3TEST",
  "bosl_imei": "12312312",
  "bosl_battery_mv": 3921,
  "bosl_bootup_timestamp": "2022-09-02T13:30:42",
  "chmln_top_raw_a": 1068,
  "chmln_top_raw_b": 1152,
  "chmln_top_raw_avg": 1080,
  "chmln_top_resistance": 123123,
  "chmln_bot_raw_a": 3224,
  "chmln_bot_raw_b": 3188,
  "chmln_bot_raw_avg": 123123,
  "chmln_bot_resistance": 123123,
  "ds18b20_bot_temperature": -9.5,
  "ds18b20_top_temperature": -7.5,
  "smt100_top_temperature": 33,
  "smt100_bot_vwc": 73,
  "smt100_bot_temperature": 34,
  "smt100_top_vwc": 72
}
*/