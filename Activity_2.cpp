#include "mbed.h"
#include <stdexcept>
#include "stdint.h"

#define LM75_REG_TEMP (0x00)
#define LM75_REG_CONF (0x01)
#define LM75_ADDR     (0x90)
#define LM75_REG_TOS (0x03)
#define LM75_REG_THYST (0x02)

I2C i2c(I2C_SDA, I2C_SCL);

// Set LEDs
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

InterruptIn lm75_int(D7); // OS connected to D7

Serial pc(SERIAL_TX, SERIAL_RX);


float t_array[60]; // Defines array for the 60s temp readings
float temperature; // Float for temperature value
int time_running;

int16_t i16; // Make i16 16 bits wide for TOS and THYST to work

Ticker record_temp_ticker;
float time_interval = 1.0;

bool stop = 0; // At start, temp hasn't reached threshold so don't stop timer and recording

void onRecordTempTicker(void)
{
    if (stop==0){ //Record data to the array for first 60 seconds
        if (time_running < 60){
            t_array[time_running] = temperature;
        }
        else{
            for (int i = 0; i < 59 ; i++){ //Refreshes and shifts the data left in the array after 60s
                t_array[i]=t_array[i+1];
                t_array[59] = temperature;
            }
        }
    }
    time_running++; //Increments time by 1s
}

void trigger_alarm() //Flash LEDs when temp too high
{
        led1=!led1;
        led2=!led2;
        led3=!led3;
        stop=1;
}

int main(){
    char data_write[3];
    char data_read[3];

    data_write[0] = LM75_REG_CONF;
    data_write[1] = 0x00;
    int status = i2c.write(LM75_ADDR, data_write, 2, 0);
    if (status != 0)
    { // Error
            while (1)
            {
                    led1 = !led1;
                    wait(0.2);
            }
    }

    float tos=28; // TOS temperature
    float thyst=26; // THYST temperature

    // This section of code sets the TOS register
    data_write[0]=LM75_REG_TOS;
    i16 = (int16_t)(tos*256) & 0xFF80;
    data_write[1]=(i16 >> 8) & 0xff;
    data_write[2]=i16 & 0xff;
    i2c.write(LM75_ADDR, data_write, 3, 0);

    // This section of code sets the THYST register
    data_write[0]=LM75_REG_THYST;
    i16 = (int16_t)(thyst*256) & 0xFF80;
    data_write[1]=(i16 >> 8) & 0xff;
    data_write[2]=i16 & 0xff;
    i2c.write(LM75_ADDR, data_write, 3, 0);

    // This line attaches the interrupt.
    // The interrupt line is active low so we trigger on a falling edge
    lm75_int.fall(&trigger_alarm);

    // Record temperature value every second
    record_temp_ticker.attach(onRecordTempTicker, time_interval);

    while(!stop){ //If temp hasn't reached threshold
        // Read temperature register
        data_write[0] = LM75_REG_TEMP;
        i2c.write(LM75_ADDR, data_write, 1, 1);
        i2c.read(LM75_ADDR, data_read, 2, 0);

        // Calculate temperature value in Celcius
        int16_t i16 = (data_read[0] << 8) | data_read[1];
        // Read data as twos complement integer so sign is correct
        temperature = i16 / 256.0;

        // Display result
        pc.printf("Temperature = %.3f\r\n",temperature);
        wait(1.0);
    }

    // If temp has reached threshold
    pc.printf("ALARM TRIGGERED");
    // Print las 60s
    for(int i = 0; i < 60; i++){
        pc.printf("Temperature "), pc.printf("%d",60-i), pc.printf(" seconds ago: %.3f\n",t_array[i]);
    }
    while(true){
        led1=false;
        led2=true;
        led3=false;
        wait(0.1);
        led2=false;
        led3=true;
        wait(0.1);
    }
}