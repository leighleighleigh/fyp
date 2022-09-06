#include <Arduino.h>

// const char *column_data_schema = "{\"bosl_name\":\"%s\",\"bosl_imei\":\"%s\",\"bosl_battery_mv\":%d,\"bosl_bootup_timestamp\":%s,\"chmln_top_raw_a\":%d,\"chmln_top_raw_b\":%d,\"chmln_top_raw_avg\":%d,\"chmln_top_resistance\":%d,\"chmln_bot_raw_a\":%d,\"chmln_bot_raw_b\":%d,\"chmln_bot_raw_avg\":%s,\"chmln_bot_resistance\":%d,\"ds18b20_bot_temperature\":%s,\"ds18b20_top_temperature\":%s,\"smt100_top_vwc\":%s,\"smt100_top_temperature\":%s,\"smt100_bot_vwc\":%d,\"smt100_bot_temperature\":%d}";
const char *column_data_schema = "{\"bosl_name\":\"%s\",\"bosl_imei\":\"%s\",\"bosl_battery_mv\":%d}";

// Let's use dweet.io's root CA:
const char root_ca[] PROGMEM = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFSDCCBDCgAwIBAgISBFGeair2czQBv7wC1AyYMbFZMA0GCSqGSIb3DQEBCwUA\n" \
"MDIxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQswCQYDVQQD\n" \
"EwJSMzAeFw0yMjA4MzEwODAwMThaFw0yMjExMjkwODAwMTdaMBcxFTATBgNVBAMT\n" \
"DHZwbi5sZWlnaC5zaDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALPO\n" \
"XwE502Oxl4lWaGVTsTBZqmWpl4tZTNHnOO4R6A/eMEoBYxHKj2Hg7bgV+aPeNzhp\n" \
"JyRR/T4S1yD47Gjb02KiNb7MBU27HBhpA9cMhZHl61/UXi6LVo/Slx90n7l5Tbrl\n" \
"dg9A46WNvyW5Pc8LuMO4g0Of8mtvanV8e9vqKIIJ8d3yr7oOTYCrgmv2Us7G6RLS\n" \
"HpD8SAQfKOTJVpNZcNeNePmD9YnuGn7q5eUkuPl5nt2haq6FcZyGP1FNxEVZXWp1\n" \
"/yvWJcHA/Y4T/VvYaxMaku2HqVgBZBn1X3XoCFZRuQbYMFgDIRych00KMT2zW67s\n" \
"k2ZcQrRmA9lYOTnLazUCAwEAAaOCAnEwggJtMA4GA1UdDwEB/wQEAwIFoDAdBgNV\n" \
"HSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDAYDVR0TAQH/BAIwADAdBgNVHQ4E\n" \
"FgQUVWM8KBkzfYcZdwSgqQ4G3ipI2O4wHwYDVR0jBBgwFoAUFC6zF7dYVsuuUAlA\n" \
"5h+vnYsUwsYwVQYIKwYBBQUHAQEESTBHMCEGCCsGAQUFBzABhhVodHRwOi8vcjMu\n" \
"by5sZW5jci5vcmcwIgYIKwYBBQUHMAKGFmh0dHA6Ly9yMy5pLmxlbmNyLm9yZy8w\n" \
"QAYDVR0RBDkwN4IMYXBwLmxlaWdoLnNoggxjbXMubGVpZ2guc2iCC21jLmxlaWdo\n" \
"LnNoggx2cG4ubGVpZ2guc2gwTAYDVR0gBEUwQzAIBgZngQwBAgEwNwYLKwYBBAGC\n" \
"3xMBAQEwKDAmBggrBgEFBQcCARYaaHR0cDovL2Nwcy5sZXRzZW5jcnlwdC5vcmcw\n" \
"ggEFBgorBgEEAdZ5AgQCBIH2BIHzAPEAdwBByMqx3yJGShDGoToJQodeTjGLGwPr\n" \
"60vHaPCQYpYG9gAAAYLzIT82AAAEAwBIMEYCIQDmXuwttmflq5LvQLBklizk6uBv\n" \
"tCIt1xVOe14G8FU14wIhAMXWDgpoEYZ6iTBFA37BmKNfEdTx6Fer8JdSP/rltrHw\n" \
"AHYAKXm+8J45OSHwVnOfY6V35b5XfZxgCvj5TV0mXCVdx4QAAAGC8yE+/wAABAMA\n" \
"RzBFAiBtbdu6Z2IHC8H1UQBCUqq6rO3PcYa7WZjGe++ESWq54QIhALixtVcFjjE6\n" \
"EaKHvsaJPqheYVIF7GjCf1N9swmgZorHMA0GCSqGSIb3DQEBCwUAA4IBAQAT+rT4\n" \
"OkOhJRGPiAUlOjj3PaTLWqz2ClkRs3CNj3jnRZA6Ct72anpIyhlYXODVD61Hs/Kv\n" \
"T/O5IGpumGZrpxrmmJYXkrSZh12/hKwSxn0nq1cRH4KatXrWyK5CcqVfncBcJApa\n" \
"EXWP8hU97rX3FXsK693vuAg5dlj55+daKt+mpm1g98j5ZdAw++BG3tzhW4+zXCYC\n" \
"1Dirk17clMa6aNqL2f+dyTt++xFe3lwjWQFdsNjYvsnaeQbFYS4V9OhjggIeFCtO\n" \
"Py4Zfq/8tEFj3UdgVXZKiUCcbyLEFK2ujNbmg4a7Tb9E3DmTY+mqQaZZCQg5yFeq\n" \
"3y6pjXbmoRqprfFR\n" \
"-----END CERTIFICATE-----\n";
