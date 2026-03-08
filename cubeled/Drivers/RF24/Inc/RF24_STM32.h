/*
 * RF24_STM32.h — nRF24L01(+) driver for STM32 HAL (C port)
 *
 * Ported from the nRF24/RF24 Arduino library
 *   https://github.com/nRF24/RF24
 *
 * ----------------------------------------------------------------------------
 * PORTABILITY
 * ----------------------------------------------------------------------------
 * By default this header includes stm32f1xx_hal.h (STM32F1 family).
 * To use a different STM32 family, define RF24_HAL_INCLUDE before including
 * this header, or pass it as a compiler flag:
 *
 *   #define RF24_HAL_INCLUDE "stm32f4xx_hal.h"   // F4 family
 *   #define RF24_HAL_INCLUDE "stm32l4xx_hal.h"   // L4 family
 *   // ... etc.
 *
 * Or via the build system (makefile / CubeIDE project settings):
 *   -DRF24_HAL_INCLUDE='"stm32f4xx_hal.h"'
 * ----------------------------------------------------------------------------
 *
 * TYPICAL WIRING (STM32F103 "Blue Pill")
 *   nRF24L01+ pin  |  Blue Pill pin
 *   ───────────────┼───────────────────────────────
 *   VCC            |  3.3 V  (add 100 µF + 100 nF caps!)
 *   GND            |  GND
 *   CE             |  PB0   (any output GPIO)
 *   CSN            |  PA4   (any output GPIO)
 *   SCK            |  PA5   (SPI1_SCK)
 *   MOSI           |  PA7   (SPI1_MOSI)
 *   MISO           |  PA6   (SPI1_MISO)
 *   IRQ            |  (not connected — polling mode)
 */

#ifndef RF24_STM32_H_
#define RF24_STM32_H_

/* ===== HAL include — override by defining RF24_HAL_INCLUDE ===== */
#ifndef RF24_HAL_INCLUDE
  #define RF24_HAL_INCLUDE "stm32f1xx_hal.h"
#endif
#include RF24_HAL_INCLUDE

#include "nRF24L01.h"
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ───────────────────────────────────────────────────────────────────────────
 * Public enumerations (mirrors the Arduino RF24 library API)
 * ─────────────────────────────────────────────────────────────────────────── */

/** PA (transmit power) levels */
typedef enum {
    RF24_PA_MIN   = 0,  /* -18 dBm */
    RF24_PA_LOW   = 1,  /* -12 dBm */
    RF24_PA_HIGH  = 2,  /*  -6 dBm */
    RF24_PA_MAX   = 3,  /*   0 dBm */
    RF24_PA_ERROR = 4
} rf24_pa_dbm_e;

/** Air data rates */
typedef enum {
    RF24_1MBPS   = 0,
    RF24_2MBPS   = 1,
    RF24_250KBPS = 2   /* nRF24L01+ only */
} rf24_datarate_e;

/** CRC length */
typedef enum {
    RF24_CRC_DISABLED = 0,
    RF24_CRC_8        = 1,
    RF24_CRC_16       = 2
} rf24_crclength_e;

/* ───────────────────────────────────────────────────────────────────────────
 * Driver handle
 * ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief All state for one nRF24L01+ device.
 *
 * Declare one per radio, initialise with RF24_init(), then call RF24_begin().
 * Pass a pointer to this struct to every RF24_* function.
 * Do not read or write fields directly — use the API functions.
 */
typedef struct {
    /* --- Hardware (set by RF24_init, do not change after RF24_begin) --- */
    SPI_HandleTypeDef *hspi;    /**< HAL SPI handle                          */
    GPIO_TypeDef      *ce_port; /**< GPIO port for CE                        */
    uint16_t           ce_pin;  /**< GPIO pin mask for CE (e.g. GPIO_PIN_0)  */
    GPIO_TypeDef      *csn_port;/**< GPIO port for CSN                       */
    uint16_t           csn_pin; /**< GPIO pin mask for CSN                   */

    /* --- Internal state (managed by the library) --- */
    uint8_t  status;                    /**< Last STATUS byte from SPI       */
    uint8_t  config_reg;                /**< Cached NRF_CONFIG value         */
    uint8_t  payload_size;              /**< Static payload size 1-32        */
    uint8_t  addr_width;                /**< Address width in bytes (3-5)    */
    uint8_t  dynamic_payloads_enabled;  /**< 1 when DPL is active            */
    uint8_t  ack_payloads_enabled;      /**< 1 when ACK payloads are active  */
    uint8_t  pipe0_reading_address[5];  /**< Saved pipe 0 RX address         */
    uint8_t  pipe0_writing_address[5];  /**< Saved pipe 0 TX address         */
    uint8_t  p0_rx_active;              /**< 1 if pipe 0 opened for reading  */
    uint8_t  is_p_variant;              /**< 1 if chip is nRF24L01+          */
} RF24_t;

/* ───────────────────────────────────────────────────────────────────────────
 * Initialisation
 * ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Populate the hardware fields of an RF24_t handle.
 *
 * Must be called before RF24_begin(). Does not communicate with the chip.
 *
 * @param radio    Pointer to an RF24_t struct.
 * @param hspi     Pointer to the HAL SPI handle (e.g. &hspi1).
 * @param ce_port  GPIO port for CE  (e.g. GPIOB).
 * @param ce_pin   GPIO pin for CE   (e.g. GPIO_PIN_0).
 * @param csn_port GPIO port for CSN (e.g. GPIOA).
 * @param csn_pin  GPIO pin for CSN  (e.g. GPIO_PIN_4).
 */
void RF24_init(RF24_t *radio,
               SPI_HandleTypeDef *hspi,
               GPIO_TypeDef *ce_port,  uint16_t ce_pin,
               GPIO_TypeDef *csn_port, uint16_t csn_pin);

/**
 * @brief Start up the radio and apply sensible defaults.
 *
 * Resets internal state, verifies SPI communication, configures the chip with
 * 1 Mbps / max PA / 16-bit CRC / auto-ack on all pipes.
 *
 * @return 1 on success (chip found and configured), 0 on failure.
 */
uint8_t RF24_begin(RF24_t *radio);

/* ───────────────────────────────────────────────────────────────────────────
 * Primary interface
 * ─────────────────────────────────────────────────────────────────────────── */

/** Switch to RX mode. Call RF24_openReadingPipe() first. */
void    RF24_startListening(RF24_t *radio);

/** Return to standby/TX mode. Call before RF24_write(). */
void    RF24_stopListening(RF24_t *radio);

/**
 * @brief Check if a payload is waiting in the RX FIFO.
 * @return 1 if data available, 0 otherwise.
 */
uint8_t RF24_available(RF24_t *radio);

/**
 * @brief Check availability and report which pipe received data.
 * @param[out] pipe_num Filled with the pipe number (0-5) on return.
 * @return 1 if data available.
 */
uint8_t RF24_availablePipe(RF24_t *radio, uint8_t *pipe_num);

/**
 * @brief Read the next payload from the RX FIFO.
 * @param buf Buffer to receive data.
 * @param len Number of bytes to read.
 */
void    RF24_read(RF24_t *radio, void *buf, uint8_t len);

/**
 * @brief Transmit a payload and wait for ACK (blocking, up to 100 ms).
 *
 * Call RF24_stopListening() and RF24_openWritingPipe() first.
 *
 * @param buf Data to transmit.
 * @param len Payload length in bytes (1-32).
 * @return 1 if acknowledged, 0 if max retransmits reached or timeout.
 */
uint8_t RF24_write(RF24_t *radio, const void *buf, uint8_t len);

/**
 * @brief Like RF24_write() but with optional per-packet ACK disable.
 * @param multicast 1 = no ACK requested, 0 = normal (ACK expected).
 *                  Requires RF24_enableDynamicAck() to have been called once.
 */
uint8_t RF24_writeMulticast(RF24_t *radio, const void *buf, uint8_t len,
                            uint8_t multicast);

/* ───────────────────────────────────────────────────────────────────────────
 * Pipe management
 * ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Set the TX address. Also sets pipe 0 RX address for auto-ack.
 * @param address Byte array of length addr_width (default 5).
 */
void RF24_openWritingPipe(RF24_t *radio, const uint8_t *address);

/**
 * @brief Open a pipe for receiving.
 * @param number Pipe number 0-5.
 * @param address Byte array of length addr_width (pipes 2-5: only first byte unique).
 */
void RF24_openReadingPipe(RF24_t *radio, uint8_t number, const uint8_t *address);

/** Close a reading pipe (disables it in EN_RXADDR). */
void RF24_closeReadingPipe(RF24_t *radio, uint8_t pipe);

/* ───────────────────────────────────────────────────────────────────────────
 * Configuration
 * ─────────────────────────────────────────────────────────────────────────── */

void             RF24_setChannel(RF24_t *radio, uint8_t channel);
uint8_t          RF24_getChannel(RF24_t *radio);

void             RF24_setPayloadSize(RF24_t *radio, uint8_t size);
uint8_t          RF24_getPayloadSize(RF24_t *radio);
uint8_t          RF24_getDynamicPayloadSize(RF24_t *radio);

void             RF24_setPALevel(RF24_t *radio, rf24_pa_dbm_e level, uint8_t lnaEnable);
rf24_pa_dbm_e    RF24_getPALevel(RF24_t *radio);

uint8_t          RF24_setDataRate(RF24_t *radio, rf24_datarate_e speed);
rf24_datarate_e  RF24_getDataRate(RF24_t *radio);

void             RF24_setCRCLength(RF24_t *radio, rf24_crclength_e length);
rf24_crclength_e RF24_getCRCLength(RF24_t *radio);
void             RF24_disableCRC(RF24_t *radio);

void    RF24_setAutoAck(RF24_t *radio, uint8_t enable);
void    RF24_setAutoAckPipe(RF24_t *radio, uint8_t pipe, uint8_t enable);
void    RF24_setRetries(RF24_t *radio, uint8_t delay, uint8_t count);
void    RF24_setAddressWidth(RF24_t *radio, uint8_t width);

void    RF24_enableDynamicPayloads(RF24_t *radio);
void    RF24_disableDynamicPayloads(RF24_t *radio);
void    RF24_enableAckPayload(RF24_t *radio);
void    RF24_disableAckPayload(RF24_t *radio);
void    RF24_enableDynamicAck(RF24_t *radio);

/**
 * @brief Load an ACK payload to be sent on the next ACK for a given pipe.
 * @param pipe  Pipe number (typically 1-5).
 * @param buf   Payload data.
 * @param len   Payload length (max 32 bytes).
 * @return 1 if loaded, 0 if TX FIFO full or ACK payloads not enabled.
 */
uint8_t RF24_writeAckPayload(RF24_t *radio, uint8_t pipe,
                             const void *buf, uint8_t len);

/* ───────────────────────────────────────────────────────────────────────────
 * Power management
 * ─────────────────────────────────────────────────────────────────────────── */

/** Wake the radio from power-down (~900 nA). Takes up to 5 ms. */
void RF24_powerUp(RF24_t *radio);

/** Enter low-power mode. Call RF24_powerUp() to resume. */
void RF24_powerDown(RF24_t *radio);

/* ───────────────────────────────────────────────────────────────────────────
 * Diagnostics
 * ─────────────────────────────────────────────────────────────────────────── */

/** @return 1 if SPI communication with the chip succeeds. */
uint8_t RF24_isChipConnected(RF24_t *radio);

/** @return 1 if the chip is an nRF24L01+ (supports 250 kbps). */
uint8_t RF24_isPVariant(RF24_t *radio);

/** @return 1 if signal ≥ -64 dBm on current channel (nRF24L01+ only). */
uint8_t RF24_testRPD(RF24_t *radio);

/** @return 1 if a carrier was detected (nRF24L01 non-+ only). */
uint8_t RF24_testCarrier(RF24_t *radio);

/** Flush TX FIFO. @return STATUS byte. */
uint8_t RF24_flush_tx(RF24_t *radio);

/** Flush RX FIFO. @return STATUS byte. */
uint8_t RF24_flush_rx(RF24_t *radio);

/** Read STATUS register via NOP transaction. @return STATUS byte. */
uint8_t RF24_getStatus(RF24_t *radio);

/* ───────────────────────────────────────────────────────────────────────────
 * Low-level register access (advanced use only)
 * ─────────────────────────────────────────────────────────────────────────── */

uint8_t RF24_read_register(RF24_t *radio, uint8_t reg);
void    RF24_read_register_buf(RF24_t *radio, uint8_t reg,
                               uint8_t *buf, uint8_t len);
void    RF24_write_register(RF24_t *radio, uint8_t reg, uint8_t value);
void    RF24_write_register_buf(RF24_t *radio, uint8_t reg,
                                const uint8_t *buf, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* RF24_STM32_H_ */
