#include "cy_pdl.h"
#include "cybsp.h"
#include "cyhal.h"
#include "cy_retarget_io.h"
#include <stdlib.h>
#include <stdio.h>
#include <max30102.h>
#include <algorithm.h>
double temp, temp1, temp_frac;
#define MAX_BRIGHTNESS 255

/* Variable for storing character read from terminal */
uint8_t uart_read_value;

int main(void)
{
    cy_rslt_t result;
    cyhal_i2c_t i2c;

    uint32_t aun_red_buffer[500];
    uint32_t aun_ir_buffer[500];
    int32_t n_ir_buffer_length; //data length
    int32_t n_sp02;             //SPO2 value
    int8_t ch_spo2_valid;       //indicator to show if the SP02 calculation is valid
    int32_t n_heart_rate;       //heart rate value
    int8_t ch_hr_valid;         //indicator to show if the heart rate calculation is valid
    uint8_t uch_dummy;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
    result = cyhal_gpio_init(P0_4, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
    result = cyhal_i2c_init(&i2c, CYBSP_I2C_SDA, CYBSP_I2C_SCL, NULL);
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                 CY_RETARGET_IO_BAUDRATE);
    /* retarget-io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    uint32_t un_min, un_max, un_prev_data;
    int32_t n_brightness;
    float f_temp;

    max_Reset();
    CyDelay(1000);

    int i;
    max_ReadReg(0, &uch_dummy);
    
    // do
    // {
    //     printf("Press any key to start conversion\n\r");
    //     CyDelay(1000);
    //     cyhal_uart_getc(&cy_retarget_io_uart_obj, &uart_read_value, 1);
    // } while(!uart_read_value);      

    cyhal_uart_getc(&cy_retarget_io_uart_obj, &uart_read_value, 1);
    uch_dummy = uart_read_value;//getchar();
    un_min = 0x3FFFF;
    un_max = 0;
    n_brightness = 0;
    
    bool status = maxim_max30102_init(i2c);
    if (status != true)
    {
        CY_ASSERT(0);
    }

    n_ir_buffer_length = 500;
    for (i = 0; i < n_ir_buffer_length; i++)
    {
        // while (cyhal_gpio_read(P12_2) == true)
        //      ;
        max_Read_FIFO((aun_red_buffer + i), (aun_ir_buffer + i));
        if (un_min > aun_red_buffer[i])
            un_min = aun_red_buffer[i]; //update signal min
        if (un_max < aun_red_buffer[i])
            un_max = aun_red_buffer[i];
        printf("red %lu ", (unsigned long)aun_red_buffer[i]);        
        printf("ir %lu\r\n", (unsigned long)aun_ir_buffer[i]);      
    }
    un_prev_data = aun_red_buffer[i];
    maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
    //max_Temperature();
    for (;;)
    {
        i = 0;
        un_min = 0x3FFFF;
        un_max = 0;

        //dumping the first 100 sets of samples in the memory and shift the last 400 sets of samples to the top
        for (i = 100; i < 500; i++)
        {
            aun_red_buffer[i - 100] = aun_red_buffer[i];
            aun_ir_buffer[i - 100] = aun_ir_buffer[i];

            //update the signal min and max
            if (un_min > aun_red_buffer[i])
                un_min = aun_red_buffer[i];
            if (un_max < aun_red_buffer[i])
                un_max = aun_red_buffer[i];
        }

        //take 100 sets of samples before calculating the heart rate.
        for (i = 400; i < 500; i++)
        {
            un_prev_data = aun_red_buffer[i - 1];
            // while (cyhal_gpio_read(P12_2) == true)
            //      ;
            max_Read_FIFO((aun_red_buffer + i), (aun_ir_buffer + i));

            if (aun_red_buffer[i] > un_prev_data)
            {
                f_temp = aun_red_buffer[i] - un_prev_data;
                f_temp /= (un_max - un_min);
                f_temp *= MAX_BRIGHTNESS;
                n_brightness -= (int)f_temp;
                if (n_brightness < 0)
                    n_brightness = 0;
            }
            else
            {
                f_temp = un_prev_data - aun_red_buffer[i];
                f_temp /= (un_max - un_min);
                f_temp *= MAX_BRIGHTNESS;
                n_brightness += (int)f_temp;
                if (n_brightness > MAX_BRIGHTNESS)
                    n_brightness = MAX_BRIGHTNESS;
            }

            //send samples and calculation result to terminal program through UART
            if ((unsigned long)ch_spo2_valid == 1)
            {
                printf("SpO2=%lu \n\r", (unsigned long)n_sp02);
            }
            // printf("red=%lu ", (unsigned long)aun_red_buffer[i]);            
            // printf("ir=%lu ", (unsigned long)aun_ir_buffer[i]);            
            // printf("HR=%lu ", (unsigned long)n_heart_rate);            
            // printf("HRvalid=%lu ", (unsigned long)ch_hr_valid);            
            // printf("SpO2=%lu ", (unsigned long)n_sp02);            
            // printf("SPO2Valid=%lu \n\r", (unsigned long)ch_spo2_valid);            
        }
        maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
    }
}