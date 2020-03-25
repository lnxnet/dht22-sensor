/*
 * sensor header file
 */

#ifndef _SENSOR_H
#define _SENSOR_H

#define MAX_TIMINGS 85
#define DHT_PIN 7
#define SAMPLE_COUNT 6
#define SAMPLE_INTERVAL 10000
#define INFLUX_URL "http://rpi3:8086/write?db=sensor_data&u=basement&p=basement"
#define INFUX_RETRY_COUNT 3
#define INFUX_RETRY_DELAY 1000

/* Types */
typedef struct {
    float temperature;
    float humidity;
} DHT_sensors;

/* Functions */
void sigterm_handler(int signum);
static void daemonize();
void send_dht_data(DHT_sensors *dht_values);
DHT_sensors* read_dht_data(void);
DHT_sensors* process_dht_data(DHT_sensors **values);
int float_cmp(const void* a, const void* b);

#endif /* _SENSOR_H */
