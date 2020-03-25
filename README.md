# dht22-sensor

DHT22 sensor daemon with influxdb integration.

Sensor reading code inspired from http://www.uugear.com/portfolio/read-dht1122-temperature-humidity-sensor-from-raspberry-pi

## Pre-requisites

- Raspbian Buster
- wiringpi
- libcurl4
- a working influxdb server

## Wiring

See https://pinout.xyz/pinout/wiringpi

- BCM4, wiringpi 7
- 3v3 (any)
- GND (any)

## To do

- Configuration file support
