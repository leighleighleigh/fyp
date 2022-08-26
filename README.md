# Sensors In Green Infrastructure
*Application of low-cost electronic sensors to the challenge of biofilter maintenance* <br>
> **Monash University** <br>
> **Engineering Final Year Project** <br>
> Author: Leigh Oliver <br>
> Supervisor: Dr. Brandon Winfrey <br>

# About this repo
My final year project is build upon the Monash-created [BoSL Board](http://www.bosl.com.au/). This board is built around the Atmega 328P microprocessor, the same as the popular Arduino Uno/Nano boards. The BoSL board also features an integrated [SIM7000G cellular modem](https://www.simcom.com/product/SIM7000G.html), capable of LTE CAT-M1 and LTE NB-IoT connectivity, allowing for internet connectivity.

Source code in this repo is best used within the [PlatformIO](https://platformio.org/) development environment.

# Directory Map
```bash
├── data # experimental data and plots
├── firmware # source-code for BoSL boards
│   └── prototype_column # 2 chameleons and 1 SMT100
│       └── bosl_json_serial_logger # JSON data written to serial every 1 second
└── tools # plot.py, used to convert JSON /data into plot images
```

# Links
[Monash BoSL Website](http://www.bosl.com.au/)<br>
[Monash BoSL GitHub](https://github.com/Monash-BoSL/Monash-BoSL)
