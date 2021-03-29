#include "cy_pdl.h"
#include "cyhal.h"
#include "max30102.h"

cyhal_i2c_t i2c;

bool maxim_max30102_init(cyhal_i2c_t _i2c_)

{
    max_Reset();
    cyhal_system_delay_ms(1000);
    i2c=_i2c_;
    if (!max_WriteReg(REG_INTR_ENABLE_1, 0xc0)) // INTR setting
        return false;
    if (!max_WriteReg(REG_INTR_ENABLE_2, 0x00))
        return false;
    if (!max_WriteReg(REG_FIFO_WR_PTR, 0x00)) //FIFO_WR_PTR[4:0]
        return false;
    if (!max_WriteReg(REG_OVF_COUNTER, 0x00)) //OVF_COUNTER[4:0]
        return false;
    if (!max_WriteReg(REG_FIFO_RD_PTR, 0x00)) //FIFO_RD_PTR[4:0]
        return false;
    if (!max_WriteReg(REG_FIFO_CONFIG, 0x0f)) //sample avg = 1, fifo rollover=false, fifo almost full = 17
        return false;
    if (!max_WriteReg(REG_MODE_CONFIG, 0x03)) //0x02 for Red only, 0x03 for SpO2 mode 0x07 multimode LED
        return false;
    if (!max_WriteReg(REG_SPO2_CONFIG, 0x27)) // SPO2_ADC range = 4096nA, SPO2 sample rate (100 Hz), LED pulseWidth (400uS)
        return false;
    if (!max_WriteReg(REG_LED1_PA, 0x24)) //Choose value for ~ 7mA for LED1
        return false;
    if (!max_WriteReg(REG_LED2_PA, 0x24)) // Choose value for ~ 7mA for LED2
        return false;
    if (!max_WriteReg(REG_PILOT_PA, 0x7f)) // Choose value for ~ 25mA for Pilot LED
        return false;
    return true;
}
bool max_WriteReg(uint8_t Reg, uint8_t Value)
{
    char Write_Buf[2] = {0};
    Write_Buf[0] = Reg;
    Write_Buf[1] = Value;
    cyhal_i2c_master_write(&i2c, I2C_ADDRESS, (uint8_t *)Write_Buf, 2, 0, true);
    return true;
}
bool max_ReadReg(uint8_t Reg, uint8_t *Value)
{ 
    /* Value pointer */
    char Data[1] = {0};
    Data[0] = Reg;
    /* Step 1 Read Modunu */
    cyhal_i2c_master_write(&i2c, I2C_ADDRESS, (uint8_t *)Data, 1, 0, true);
    /* Step 2 Read */
    cyhal_i2c_master_read(&i2c, I2C_ADDRESS, (uint8_t *)Data, 1, 0, true);
    *Value = Data[0];
    return true;
}

bool max_Reset(void)
{
    if (!max_WriteReg(REG_MODE_CONFIG, 0x40))
        return false;
    else
        return true;
}
bool max_Read_FIFO(uint32_t *pun_red_led, uint32_t *pun_ir_led)
{
    uint32_t un_temp;
    unsigned char uch_temp;
    *pun_red_led = 0;
    *pun_ir_led = 0;
    char ach_i2c_data[6];

    //read and clear status register
    max_ReadReg(REG_INTR_STATUS_1, &uch_temp);
    max_ReadReg(REG_INTR_STATUS_2, &uch_temp);

    ach_i2c_data[0] = REG_FIFO_DATA;
    /* Step 1 Read Modunu Açıyor */
    cyhal_i2c_master_write(&i2c, I2C_ADDRESS, (uint8_t *)ach_i2c_data, 1, 0, true);    
    /* Step 2 Read Ediyor */
    cyhal_i2c_master_read(&i2c, I2C_ADDRESS, (uint8_t *)ach_i2c_data, 6, 0, true);

    un_temp = (unsigned char)ach_i2c_data[0];
    un_temp <<= 16;
    *pun_red_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[1];
    un_temp <<= 8;
    *pun_red_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[2];
    *pun_red_led += un_temp;

    un_temp = (unsigned char)ach_i2c_data[3];
    un_temp <<= 16;
    *pun_ir_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[4];
    un_temp <<= 8;
    *pun_ir_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[5];
    *pun_ir_led += un_temp;
    *pun_red_led &= 0x03FFFF; //Mask MSB [23:18]
    *pun_ir_led &= 0x03FFFF;  //Mask MSB [23:18]

    return true;
}

void max_Temperature()
{
    uint8_t temp, temp1;
    float temp_frac, temp_int;
    char str[15];
    max_WriteReg(REG_TEMP_CONFIG, 0x01);
    max_ReadReg(REG_TEMP_INTR, &temp); // pointerdaki datayı &temp adresine yazıyor
    max_WriteReg(REG_TEMP_CONFIG, 0x01);
    max_ReadReg(REG_TEMP_FRAC, &temp1);
    // temp_int = temp;
    // temp_frac = temp1;
    // sprintf(str, "%.f.", temp_int);
    // UART_1_PutString(str);
    // sprintf(str, "%.f", temp_frac);
    // UART_1_PutString(str);
    // UART_1_PutCRLF(0);
}
