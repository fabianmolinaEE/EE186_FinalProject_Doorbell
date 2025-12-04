# EE186_FinalProject_Doorbell

## OLED Driver 
The part we are working with is: 2pcs 0.96" HD IPS TFT LCD Display Module 80x160 RGB LCD Screen 3.3V ST7735 Drive SPI Interface 8pin Full Color for Arduino DIY

### Pins used (right side is screen's respective pin)
* GND
* 3v3 -> VCC, BLK
* PA5 -> SCL 
* PA7 -> SDA
* PD8 -> RES
* PD9 -> DC
* PA4 -> CS


# Camera OV7670 Driver

# Pins used 
* PA4 -> DCMI_HSYNC
* PA6 -> DCMI_PIXCLK
* PB7 -> DCMI_VSYNC 
* PE6 -> DCMI_D7
* PE5 -> DCMI_D6
* PB6 -> DCMI_D5
* PC11 -> DCMI_D4
* PC9 -> DCMI_D3
* PC7 -> DCMI_D1
* PC6 -> DCMI_D0
* PB8 -> I2C1_SCL
* PB9 -> I2C1_SDA
* PA8 -> MCO
* PG7 -> LPUART_TX (For debug)
* PG8 -> LPUART_RX (For debug)
* PB14 -> Red LED (For debug)

# R307S fingerprint sensor
* Used USART2
* PB10

# PIR Sensor BS612
* PF12 -> REL
* GND -> SENSE, VSS, ONTIME
* 3.3V -> OEN, VDD

