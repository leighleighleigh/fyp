# Sensors In Green Infrastructure
*Application of low-cost electronic sensors to the challenge of biofilter maintenance* <br>
> **Monash University** <br>
> **Engineering Final Year Project** <br>
> Author: Leigh Oliver <br>
> Supervisor: Dr. Brandon Winfrey <br>
> Date completed: 29/10/2022 <br>

# About this repo
My final year project is build upon the Monash-created [BoSL Board](http://www.bosl.com.au/). This board is built around the Atmega 328P microprocessor, the same as the popular Arduino Uno/Nano boards. The BoSL board also features an integrated [SIM7000G cellular modem](https://www.simcom.com/product/SIM7000G.html), capable of LTE CAT-M1 and LTE NB-IoT connectivity, allowing for internet connectivity.

Source code in this repo is best used within the [PlatformIO](https://platformio.org/) development environment.

# Directory Map
```bash
├── docs # Useful PDFs
├── experimental_measurements # Data from falling-head test
├── firmware # Software for microcontrollers
│   ├── ArduinoISP # Used to recover BoSL boards, using another ATmega32u4
│   ├── bosl_sketches # Legacy Arduino sketched
│   └── bosl_static_json_http # Deployed code, logs to BoSL website
├── kicad # Hardware design
│   └── sensor_wiring # Schematic PDF is in here too!
└── tools # Python scripts
    ├── data # Raw data is downloaded here
    ├── data_split # Data is processed into here, as CSV
    ├── plotjuggler # Useful .XML layouts for PlotJuggler tool
    └── web # PHP scripts
```

# Links
[Monash BoSL Website](http://www.bosl.com.au/)<br>
[Monash BoSL GitHub](https://github.com/Monash-BoSL/Monash-BoSL)
