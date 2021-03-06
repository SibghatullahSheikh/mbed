/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
//==============================================================================
// STM32F103
//==============================================================================
#include "spi_api.h"

#if DEVICE_SPI

#include <math.h>
#include "cmsis.h"
#include "pinmap.h"
#include "error.h"

static const PinMap PinMap_SPI_MOSI[] = {
    {PA_7,  SPI_1, STM_PIN_DATA(GPIO_Mode_AF_PP, 0)},
    {PB_5,  SPI_1, STM_PIN_DATA(GPIO_Mode_AF_PP, 1)}, // Remap
    {NC,    NC,    0}
};

static const PinMap PinMap_SPI_MISO[] = {
    {PA_6,  SPI_1, STM_PIN_DATA(GPIO_Mode_AF_PP, 0)},
    {PB_4,  SPI_1, STM_PIN_DATA(GPIO_Mode_AF_PP, 1)}, // Remap
    {NC,    NC,    0}
};

static const PinMap PinMap_SPI_SCLK[] = {
    {PA_5,  SPI_1, STM_PIN_DATA(GPIO_Mode_AF_PP, 0)},
    {PB_3,  SPI_1, STM_PIN_DATA(GPIO_Mode_AF_PP, 1)}, // Remap
    {NC,    NC,    0}
};

// Only used in Slave mode
static const PinMap PinMap_SPI_SSEL[] = {
    {PA_4,  SPI_1, STM_PIN_DATA(GPIO_Mode_IN_FLOATING, 0)},
    {PA_15, SPI_1, STM_PIN_DATA(GPIO_Mode_IN_FLOATING, 1)}, // Remap
    {NC,    NC,    0}
};

void spi_init(spi_t *obj, PinName mosi, PinName miso, PinName sclk, PinName ssel) {

    SPI_TypeDef *spi;
    SPI_InitTypeDef SPI_InitStructure;
  
    // Determine the SPI to use
    SPIName spi_mosi = (SPIName)pinmap_peripheral(mosi, PinMap_SPI_MOSI);
    SPIName spi_miso = (SPIName)pinmap_peripheral(miso, PinMap_SPI_MISO);
    SPIName spi_sclk = (SPIName)pinmap_peripheral(sclk, PinMap_SPI_SCLK);
    SPIName spi_ssel = (SPIName)pinmap_peripheral(ssel, PinMap_SPI_SSEL);
  
    SPIName spi_data = (SPIName)pinmap_merge(spi_mosi, spi_miso);
    SPIName spi_cntl = (SPIName)pinmap_merge(spi_sclk, spi_ssel);
  
    obj->spi = (SPIName)pinmap_merge(spi_data, spi_cntl);
  
    if (obj->spi == (SPIName)NC) {
        error("SPI pinout mapping failed");
    }

    // Get SPI registers structure address
    spi = (SPI_TypeDef *)(obj->spi);
    
    // Enable SPI clock
    if (obj->spi == SPI_1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE); 
    }
    if (obj->spi == SPI_2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE); 
    }
    
    // Configure the SPI pins
    pinmap_pinout(mosi, PinMap_SPI_MOSI);
    pinmap_pinout(miso, PinMap_SPI_MISO);
    pinmap_pinout(sclk, PinMap_SPI_SCLK);
    
    // Save new values
    obj->bits = SPI_DataSize_8b;
    obj->cpol = SPI_CPOL_Low;
    obj->cpha = SPI_CPHA_1Edge;
    obj->br_presc = SPI_BaudRatePrescaler_64; // Closest to 1MHz (72MHz/64 = 1.125MHz)
    
    if (ssel == NC) { // Master
        obj->mode = SPI_Mode_Master;
        obj->nss = SPI_NSS_Soft;
    }
    else { // Slave
        pinmap_pinout(ssel, PinMap_SPI_SSEL);
        obj->mode = SPI_Mode_Slave;
        obj->nss = SPI_NSS_Hard;
    }

    // SPI configuration    
    SPI_InitStructure.SPI_Mode = obj->mode;
    SPI_InitStructure.SPI_NSS = obj->nss;    
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;    
    SPI_InitStructure.SPI_DataSize = obj->bits;
    SPI_InitStructure.SPI_CPOL = obj->cpol;
    SPI_InitStructure.SPI_CPHA = obj->cpha;    
    SPI_InitStructure.SPI_BaudRatePrescaler = obj->br_presc;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(spi, &SPI_InitStructure);

    SPI_Cmd(spi, ENABLE);    
}

void spi_free(spi_t *obj) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    SPI_I2S_DeInit(spi);
}

void spi_format(spi_t *obj, int bits, int mode, int slave) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    SPI_InitTypeDef SPI_InitStructure;

    // Save new values
  
    if (bits == 8) {
        obj->bits = SPI_DataSize_8b;
    }
    else {
        obj->bits = SPI_DataSize_16b;
    }
    
    switch (mode) {
        case 0:
          obj->cpol = SPI_CPOL_Low;
          obj->cpha = SPI_CPHA_1Edge;
        break;
        case 1:
          obj->cpol = SPI_CPOL_Low;
          obj->cpha = SPI_CPHA_2Edge;
        break;
        case 2:
          obj->cpol = SPI_CPOL_High;
          obj->cpha = SPI_CPHA_1Edge;          
        break;
        default:
          obj->cpol = SPI_CPOL_High;
          obj->cpha = SPI_CPHA_2Edge;          
        break;
    }
    
    if (slave == 0) {
        obj->mode = SPI_Mode_Master;
        obj->nss = SPI_NSS_Soft;
    }
    else {
        obj->mode = SPI_Mode_Slave;
        obj->nss = SPI_NSS_Hard;      
    }
    
    SPI_Cmd(spi, DISABLE);

    SPI_InitStructure.SPI_Mode = obj->mode;
    SPI_InitStructure.SPI_NSS = obj->nss;    
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;    
    SPI_InitStructure.SPI_DataSize = obj->bits;
    SPI_InitStructure.SPI_CPOL = obj->cpol;
    SPI_InitStructure.SPI_CPHA = obj->cpha;    
    SPI_InitStructure.SPI_BaudRatePrescaler = obj->br_presc;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;    
    SPI_Init(spi, &SPI_InitStructure);

    SPI_Cmd(spi, ENABLE);
}

void spi_frequency(spi_t *obj, int hz) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    SPI_InitTypeDef SPI_InitStructure;

    // Get SPI clock frequency
    uint32_t PCLK = SystemCoreClock >> 1;

    // Choose the baud rate divisor (between 2 and 256)
    uint32_t divisor = PCLK / hz;

    // Find the nearest power-of-2
    divisor = (divisor > 0 ? divisor-1 : 0);
    divisor |= divisor >> 1;
    divisor |= divisor >> 2;
    divisor |= divisor >> 4;
    divisor |= divisor >> 8;
    divisor |= divisor >> 16;
    divisor++;

    uint32_t baud_rate = __builtin_ffs(divisor) - 2;
    
    // Save new value
    obj->br_presc = ((baud_rate > 7) ? (7 << 3) : (baud_rate << 3));
 
    SPI_Cmd(spi, DISABLE);
    
    SPI_InitStructure.SPI_Mode = obj->mode;
    SPI_InitStructure.SPI_NSS = obj->nss;    
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;    
    SPI_InitStructure.SPI_DataSize = obj->bits;
    SPI_InitStructure.SPI_CPOL = obj->cpol;
    SPI_InitStructure.SPI_CPHA = obj->cpha;    
    SPI_InitStructure.SPI_BaudRatePrescaler = obj->br_presc;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;    
    SPI_Init(spi, &SPI_InitStructure);

    SPI_Cmd(spi, ENABLE);
}

static inline int ssp_readable(spi_t *obj) {
    int status;
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    // Check if data is received
    status = ((SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE) != RESET) ? 1 : 0);
    return status;  
}

static inline int ssp_writeable(spi_t *obj) {
    int status;
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    // Check if data is transmitted
    status = ((SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) != RESET) ? 1 : 0);
    return status;
}

static inline void ssp_write(spi_t *obj, int value) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);  
    while (!ssp_writeable(obj));
    SPI_I2S_SendData(spi, (uint16_t)value);
}

static inline int ssp_read(spi_t *obj) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);   
    while (!ssp_readable(obj));
    return (int)SPI_I2S_ReceiveData(spi);
}

static inline int ssp_busy(spi_t *obj) {
    int status;
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    status = ((SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_BSY) != RESET) ? 1 : 0);
    return status;
}

int spi_master_write(spi_t *obj, int value) {
    ssp_write(obj, value);
    return ssp_read(obj);
}

int spi_slave_receive(spi_t *obj) {
    return (ssp_readable(obj) && !ssp_busy(obj)) ? (1) : (0);
};

int spi_slave_read(spi_t *obj) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    return (int)SPI_I2S_ReceiveData(spi);
}

void spi_slave_write(spi_t *obj, int value) {
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);  
    while (!ssp_writeable(obj));  
    SPI_I2S_SendData(spi, (uint16_t)value);
}

int spi_busy(spi_t *obj) {
    return ssp_busy(obj);
}

#endif
