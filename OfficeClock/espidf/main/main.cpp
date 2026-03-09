///*-------------------------------------------------------------------------
//    This source file is a part of Clocks
//    For the latest info, see https://github.com/cmarrin/Clocks
//    Copyright (c) 2021-2026, Chris Marrin
//    All rights reserved.
//    Use of this source code is governed by the MIT license that can be
//    found in the LICENSE file.
//-------------------------------------------------------------------------*/
//
///*
//    Office Clock
//    
//    This is a thin wrapper around the Application layer of ESPlib
//*/
//
//#include "OfficeClock.h"
//
//#include "IDFWiFiPortal.h"
//
//mil::IDFWiFiPortal portal;
//OfficeClock officeClock(&portal);
//
//extern "C" {
//void app_main(void)
//{
//    officeClock.setup();
//
//    while (true) {
//        officeClock.loop();
//        vTaskDelay(1);
//    }
//}
//}
//
//
//
//
//
//













#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
#include <max7219.h>

#define HOST SPI2_HOST

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

#define CONFIG_EXAMPLE_CASCADE_SIZE 4
#define CONFIG_EXAMPLE_PIN_NUM_MOSI 4
#define CONFIG_EXAMPLE_PIN_NUM_CLK 1
#define CONFIG_EXAMPLE_PIN_CS 5

static uint8_t symbols[] =
{
    0x38, 0x38, 0x38, 0xfe, 0x7c, 0x38, 0x10, 0x00, // arrows
    0x10, 0x38, 0x7c, 0xfe, 0x38, 0x38, 0x38, 0x00,
    0x10, 0x30, 0x7e, 0xfe, 0x7e, 0x30, 0x10, 0x00,
    0x10, 0x18, 0xfc, 0xfe, 0xfc, 0x18, 0x10, 0x00,
    0x10, 0x38, 0x7c, 0xfe, 0xfe, 0xee, 0x44, 0x00, // heart
    0x10, 0x54, 0x38, 0xee, 0x38, 0x54, 0x10, 0x00, // sun
    0x7e, 0x18, 0x18, 0x18, 0x1c, 0x18, 0x18, 0x00, // digits
    0x7e, 0x06, 0x0c, 0x30, 0x60, 0x66, 0x3c, 0x00,
    0x3c, 0x66, 0x60, 0x38, 0x60, 0x66, 0x3c, 0x00,
    0x30, 0x30, 0x7e, 0x32, 0x34, 0x38, 0x30, 0x00,
    0x3c, 0x66, 0x60, 0x60, 0x3e, 0x06, 0x7e, 0x00,
    0x3c, 0x66, 0x66, 0x3e, 0x06, 0x66, 0x3c, 0x00,
    0x18, 0x18, 0x18, 0x30, 0x30, 0x66, 0x7e, 0x00,
    0x3c, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x3c, 0x00,
    0x3c, 0x66, 0x60, 0x7c, 0x66, 0x66, 0x3c, 0x00,
    0x3c, 0x66, 0x66, 0x6e, 0x76, 0x66, 0x3c, 0x00
};
static const size_t symbols_size = sizeof(symbols) - sizeof(uint64_t) * CONFIG_EXAMPLE_CASCADE_SIZE;

static void flip(uint8_t* a, int size)
{
    for (int i = 0; i < size; ++i) {
        uint8_t in = a[i];
        uint8_t out = 0;
        for (int j = 0; j < 8; j++) {
            out <<= 1;
            if (in & 0x01) {
                out |= 1;
            }
            in >>= 1;
        }
        a[i] = out;
    }
}

void task(void *pvParameter)
{
    // Configure SPI bus
    spi_bus_config_t cfg =
    {
        .mosi_io_num = CONFIG_EXAMPLE_PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = CONFIG_EXAMPLE_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HOST, &cfg, SPI_DMA_CH_AUTO));

    // Configure device
    max7219_t dev =
    {
        .digits = 0,
        .cascade_size = CONFIG_EXAMPLE_CASCADE_SIZE,
        .mirrored = true
    };
    ESP_ERROR_CHECK(max7219_init_desc(&dev, HOST, MAX7219_MAX_CLOCK_SPEED_HZ, (gpio_num_t) CONFIG_EXAMPLE_PIN_CS));
    ESP_ERROR_CHECK(max7219_init(&dev));

    flip(symbols, sizeof(symbols));


    size_t offs = 0;
    while (1)
    {
        max7219_set_digit(&dev, 31 - offs, 0xff);
        max7219_set_digit(&dev, 23 - offs, 0xff);
        max7219_set_digit(&dev, 15 - offs, 0xff);
        max7219_set_digit(&dev,  7 - offs, 0xff);

        // for (uint8_t c = 0; c < CONFIG_EXAMPLE_CASCADE_SIZE; c ++) {
        //     max7219_draw_image_8x8(&dev, c * 8, symbols + (c * 8 + offs));
        // }
        vTaskDelay(pdMS_TO_TICKS(500));

        if (++offs >= 8) {
            offs = 0;
            max7219_clear(&dev);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

extern "C" {
void app_main()
{
    xTaskCreatePinnedToCore(task, "task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL, APP_CPU_NUM);
}
}
