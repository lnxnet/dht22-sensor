/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * sensor.c
 * Get temp/hum from a DHT22 sensor and push it to an influxdb instance
 */

#define _GNU_SOURCE
#include "sensor.h"
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <syslog.h>
#include <sys/types.h>
#include <curl/curl.h>

void sigterm_handler(int signum) {
    syslog(LOG_NOTICE, "%s", strsignal(signum));
    closelog();
    exit(EXIT_SUCCESS);
}

static void daemonize() {
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);
    /* Catch, ignore and handle signals */
    signal(SIGTERM, sigterm_handler);
    signal(SIGHUP, SIG_IGN);
    /* Fork off for the second time*/
    pid = fork();
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Open the log file */
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("dht22-sensor", LOG_CONS|LOG_PID|LOG_NDELAY, LOG_DAEMON);

    if (wiringPiSetup() == -1) {
        syslog(LOG_ERR, "Couldn't initialise wiringPi");
        exit(EXIT_FAILURE);
    }
}

void send_dht_data(DHT_sensors *dht_values) {
    char post_data[80] = {0};
    uint8_t retry = 0;
    CURL *curl;

    sprintf(post_data,
            "dht22,location=basement temperature=%.1f,humidity=%.1f", 
            dht_values->temperature, dht_values->humidity);

    curl = curl_easy_init();
    if(curl) {
        CURLcode res = -1;
        struct curl_slist* headers = NULL;
        
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charset: utf-8");

        curl_easy_setopt(curl, CURLOPT_URL, INFLUX_URL);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

        while (res != CURLE_OK && retry < INFUX_RETRY_COUNT) {
        res = curl_easy_perform(curl);
            if(res) {
                const char *str = curl_easy_strerror(res);
                syslog(LOG_WARNING, "libcurl: %s", str);
                retry++;
                delay(INFUX_RETRY_DELAY);
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

DHT_sensors* read_dht_data() {
    int dht_data[5] = {0, 0, 0, 0, 0};
    DHT_sensors *sensor = NULL;
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    dht_data[0] = dht_data[1] = dht_data[2] = dht_data[3] = dht_data[4] = 0;

    /* pull pin down for 18 milliseconds */
    pinMode(DHT_PIN, OUTPUT);
    digitalWrite(DHT_PIN, LOW);
    delay(18);

    /* prepare to read the pin */
    pinMode(DHT_PIN, INPUT);

    /* detect change and read data */
    for (i = 0; i < MAX_TIMINGS; i++) {
        counter = 0;
        while (digitalRead(DHT_PIN) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255)
                break;
        }
        laststate = digitalRead(DHT_PIN);

        if (counter == 255)
            break;

        /* ignore first 3 transitions */
        if ((i >= 4) && (i % 2 == 0)) {
            /* shove each bit into the storage bytes */
            dht_data[j / 8] <<= 1;
            if (counter > 16)
                dht_data[j / 8] |= 1;
            j++;
        }
    }

    /*
     * check we read 40 bits (8bit x 5) + verify checksum in the last byte
     */
    if ((j >= 40) &&
         (dht_data[4] == ((dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) & 0xFF))) {
        float t = (float)(((dht_data[2] & 0x7F) << 8) + dht_data[3]) / 10;
        float h = (float)((dht_data[0] << 8) + dht_data[1]) / 10;
        if (dht_data[2] & 0x80)
            t = -t;

        sensor = malloc(sizeof(DHT_sensors));
        sensor->temperature = t;
        sensor->humidity = h;
    }
    return sensor;
}

DHT_sensors* process_dht_data(DHT_sensors **values) {
    float temperatures[SAMPLE_COUNT] = {0.0};
    float humidities[SAMPLE_COUNT] = {0.0};
    float mean_temp = 0.0;
    float mean_hum = 0.0;
    DHT_sensors *mean = NULL;
    uint8_t i = 0;

    mean = malloc(sizeof(DHT_sensors));
    for (i = 0;i < SAMPLE_COUNT; i++) {
        temperatures[i] = values[i]->temperature;
        humidities[i] = values[i]->humidity;
    }

    qsort(temperatures, SAMPLE_COUNT, sizeof(float), float_cmp);
    qsort(humidities, SAMPLE_COUNT, sizeof(float), float_cmp);
    for (i = 1; i < (SAMPLE_COUNT-1); i++) {
        mean_temp += temperatures[i];
        mean_hum += humidities[i];
    }
    mean_temp = mean_temp / (float)(SAMPLE_COUNT-2);
    mean_hum = mean_hum / (float)(SAMPLE_COUNT-2);
    mean->temperature = mean_temp;
    mean->humidity = mean_hum;

    return mean;
}

int float_cmp(const void* a, const void* b) {
    const float pa = *(const float *)a;
    const float pb = *(const float *)b;

    if(pa < pb)
        return -1;
    if(pa > pb)
        return 1;

    return 0;
}

int main() {
    DHT_sensors *dht_values[SAMPLE_COUNT];
    DHT_sensors *current_value = NULL;
    DHT_sensors *dht_to_send = NULL;
    uint8_t i = 0, val_count = 0;

    daemonize();

    syslog(LOG_NOTICE, "basement_sensor initialised");
    while (1) {
        current_value = read_dht_data();
        if (current_value != NULL) {
            dht_values[val_count] = current_value;
            current_value = NULL;
            val_count++;
        }
        if (val_count >= SAMPLE_COUNT) {
            dht_to_send = process_dht_data(dht_values);
            send_dht_data(dht_to_send);
            free(dht_to_send);
            for (i = 0;i < SAMPLE_COUNT; i++)
                free(dht_values[i]);
            val_count = 0;
        }
        delay(SAMPLE_INTERVAL);
    }
    syslog(LOG_NOTICE, "basement_sensor terminated");

    closelog();
    exit(EXIT_SUCCESS);
}
