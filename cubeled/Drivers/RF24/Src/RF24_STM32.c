/*
 * RF24_STM32.c — nRF24L01(+) driver for STM32 HAL
 *
 * Ported from the nRF24/RF24 Arduino library
 *   https://github.com/nRF24/RF24  (MIT / GPL-v2)
 *
 * All SPI communication uses STM32 HAL_SPI_* functions.
 * All GPIO toggling uses HAL_GPIO_WritePin().
 * Microsecond delays use the ARM DWT cycle counter (available on
 * Cortex-M3/M4/M7).  RF24_begin() enables DWT automatically.
 */

#include "RF24_STM32.h"

/* ─────────────────────────────────────────────────────────────────────────
 * Internal helpers
 * ───────────────────────────────────────────────────────────────────────── */

static inline void _ce(RF24_t *r, uint8_t high) {
    HAL_GPIO_WritePin(r->ce_port, r->ce_pin,
                      high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline void _csn(RF24_t *r, uint8_t high) {
    HAL_GPIO_WritePin(r->csn_port, r->csn_pin,
                      high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void _delay_us(uint32_t us) {
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = us * (HAL_RCC_GetHCLKFreq() / 1000000U);
    while ((DWT->CYCCNT - start) < cycles) { /* busy wait */ }
}

static uint8_t _spi_xfer(RF24_t *r, uint8_t byte) {
    uint8_t rx;
    HAL_SPI_TransmitReceive(r->hspi, &byte, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Low-level register read / write
 * ───────────────────────────────────────────────────────────────────────── */

uint8_t RF24_read_register(RF24_t *r, uint8_t reg) {
    uint8_t result;
    _csn(r, 0);
    r->status = _spi_xfer(r, R_REGISTER | (REGISTER_MASK & reg));
    result    = _spi_xfer(r, RF24_NOP);
    _csn(r, 1);
    return result;
}

void RF24_read_register_buf(RF24_t *r, uint8_t reg, uint8_t *buf, uint8_t len) {
    _csn(r, 0);
    r->status = _spi_xfer(r, R_REGISTER | (REGISTER_MASK & reg));
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = _spi_xfer(r, RF24_NOP);
    }
    _csn(r, 1);
}

void RF24_write_register(RF24_t *r, uint8_t reg, uint8_t value) {
    _csn(r, 0);
    r->status = _spi_xfer(r, W_REGISTER | (REGISTER_MASK & reg));
    _spi_xfer(r, value);
    _csn(r, 1);
}

void RF24_write_register_buf(RF24_t *r, uint8_t reg,
                             const uint8_t *buf, uint8_t len) {
    _csn(r, 0);
    r->status = _spi_xfer(r, W_REGISTER | (REGISTER_MASK & reg));
    for (uint8_t i = 0; i < len; i++) {
        _spi_xfer(r, buf[i]);
    }
    _csn(r, 1);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Internal payload helpers
 * ───────────────────────────────────────────────────────────────────────── */

static void _write_payload(RF24_t *r, const void *buf, uint8_t len, uint8_t cmd) {
    uint8_t blank = 0;
    uint8_t data_len  = (len < r->payload_size) ? len : r->payload_size;
    uint8_t blank_len = r->dynamic_payloads_enabled ?
                        0 : (r->payload_size - data_len);
    _csn(r, 0);
    r->status = _spi_xfer(r, cmd);
    const uint8_t *p = (const uint8_t *)buf;
    for (uint8_t i = 0; i < data_len; i++)  { _spi_xfer(r, p[i]);  }
    while (blank_len--)                       { _spi_xfer(r, blank); }
    _csn(r, 1);
}

static void _read_payload(RF24_t *r, void *buf, uint8_t len) {
    uint8_t data_len  = (len < r->payload_size) ? len : r->payload_size;
    uint8_t blank_len = r->dynamic_payloads_enabled ?
                        0 : (r->payload_size - data_len);
    _csn(r, 0);
    r->status = _spi_xfer(r, R_RX_PAYLOAD);
    uint8_t *p = (uint8_t *)buf;
    for (uint8_t i = 0; i < data_len; i++) { p[i] = _spi_xfer(r, RF24_NOP); }
    while (blank_len--)                     { _spi_xfer(r, RF24_NOP); }
    _csn(r, 1);
}

static void _toggle_features(RF24_t *r) {
    _csn(r, 0);
    _spi_xfer(r, ACTIVATE);
    _spi_xfer(r, 0x73);
    _csn(r, 1);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Initialisation
 * ───────────────────────────────────────────────────────────────────────── */

void RF24_init(RF24_t *r,
               SPI_HandleTypeDef *hspi,
               GPIO_TypeDef *ce_port,  uint16_t ce_pin,
               GPIO_TypeDef *csn_port, uint16_t csn_pin) {
    memset(r, 0, sizeof(RF24_t));
    r->hspi      = hspi;
    r->ce_port   = ce_port;
    r->ce_pin    = ce_pin;
    r->csn_port  = csn_port;
    r->csn_pin   = csn_pin;
    r->payload_size = 32;
    r->addr_width   = 5;
}

uint8_t RF24_begin(RF24_t *r) {
    /* Enable DWT cycle counter for microsecond delays */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    _ce(r, 0);
    _csn(r, 1);
    HAL_Delay(5); /* power-on reset */

    /* Verify chip responds */
    RF24_write_register(r, SETUP_RETR, 0x5F); /* 1500 µs delay, 15 retries */
    if (RF24_read_register(r, SETUP_RETR) != 0x5F) {
        return 0;
    }

    /* Unlock FEATURE register (needed on nRF24L01 non-+) */
    _toggle_features(r);
    RF24_write_register(r, FEATURE, 0);
    RF24_write_register(r, DYNPD,   0);
    r->dynamic_payloads_enabled = 0;
    r->ack_payloads_enabled     = 0;

    /* Clear status flags */
    RF24_write_register(r, NRF_STATUS,
                        (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT));

    RF24_setChannel(r, 76);
    RF24_setPayloadSize(r, 32);
    RF24_setAddressWidth(r, 5);

    RF24_flush_rx(r);
    RF24_flush_tx(r);

    /* Power up, PTX, 2-byte CRC */
    r->config_reg = (1 << EN_CRC) | (1 << CRCO) | (1 << PWR_UP);
    RF24_write_register(r, NRF_CONFIG, r->config_reg);
    HAL_Delay(5);

    /* Detect nRF24L01+ variant (can do 250 kbps) */
    {
        rf24_datarate_e saved = RF24_getDataRate(r);
        r->is_p_variant = RF24_setDataRate(r, RF24_250KBPS);
        RF24_setDataRate(r, saved);
    }

    RF24_setDataRate(r, RF24_1MBPS);
    RF24_setPALevel(r, RF24_PA_MAX, 1);

    return 1;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Primary interface
 * ───────────────────────────────────────────────────────────────────────── */

void RF24_startListening(RF24_t *r) {
    r->config_reg |= (1 << PRIM_RX);
    RF24_write_register(r, NRF_CONFIG, r->config_reg);
    RF24_write_register(r, NRF_STATUS,
                        (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT));

    if (r->p0_rx_active) {
        RF24_write_register_buf(r, RX_ADDR_P0,
                                r->pipe0_reading_address, r->addr_width);
    }

    _ce(r, 1);
    _delay_us(130);
}

void RF24_stopListening(RF24_t *r) {
    _ce(r, 0);
    _delay_us(r->ack_payloads_enabled ? 200U : 0U);

    if (r->ack_payloads_enabled) {
        RF24_flush_tx(r);
    }

    r->config_reg &= ~(1 << PRIM_RX);
    RF24_write_register(r, NRF_CONFIG, r->config_reg);

    RF24_write_register(r, EN_RXADDR,
        RF24_read_register(r, EN_RXADDR) | (1 << ERX_P0));

    RF24_write_register_buf(r, RX_ADDR_P0,
                            r->pipe0_writing_address, r->addr_width);
}

uint8_t RF24_available(RF24_t *r) {
    return (RF24_read_register(r, FIFO_STATUS) & (1 << RX_EMPTY)) == 0;
}

uint8_t RF24_availablePipe(RF24_t *r, uint8_t *pipe_num) {
    uint8_t status = RF24_getStatus(r);
    if (pipe_num) {
        *pipe_num = (status >> RX_P_NO) & 0x07;
    }
    return (RF24_read_register(r, FIFO_STATUS) & (1 << RX_EMPTY)) == 0;
}

void RF24_read(RF24_t *r, void *buf, uint8_t len) {
    _read_payload(r, buf, len);
    RF24_write_register(r, NRF_STATUS, (1 << RX_DR));
}

uint8_t RF24_writeMulticast(RF24_t *r, const void *buf,
                            uint8_t len, uint8_t multicast) {
    uint8_t cmd = multicast ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD;
    _write_payload(r, buf, len, cmd);

    _ce(r, 1);
    _delay_us(15);
    _ce(r, 0);

    uint32_t deadline = HAL_GetTick() + 100;
    uint8_t status;
    do {
        status = RF24_getStatus(r);
        if (HAL_GetTick() > deadline) {
            RF24_flush_tx(r);
            return 0;
        }
    } while (!(status & ((1 << TX_DS) | (1 << MAX_RT))));

    RF24_write_register(r, NRF_STATUS, (1 << TX_DS) | (1 << MAX_RT));

    if (status & (1 << MAX_RT)) {
        RF24_flush_tx(r);
        return 0;
    }
    return 1;
}

uint8_t RF24_write(RF24_t *r, const void *buf, uint8_t len) {
    return RF24_writeMulticast(r, buf, len, 0);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Pipe management
 * ───────────────────────────────────────────────────────────────────────── */

void RF24_openWritingPipe(RF24_t *r, const uint8_t *address) {
    memcpy(r->pipe0_writing_address, address, r->addr_width);
    RF24_write_register_buf(r, RX_ADDR_P0, address, r->addr_width);
    RF24_write_register_buf(r, TX_ADDR,    address, r->addr_width);
    RF24_write_register(r, RX_PW_P0, r->payload_size);
}

void RF24_openReadingPipe(RF24_t *r, uint8_t number, const uint8_t *address) {
    if (number > 5) return;

    if (number == 0) {
        memcpy(r->pipe0_reading_address, address, r->addr_width);
        r->p0_rx_active = 1;
    }

    if (number <= 1) {
        RF24_write_register_buf(r, RX_ADDR_P0 + number, address, r->addr_width);
    } else {
        RF24_write_register(r, RX_ADDR_P0 + number, address[0]);
    }

    RF24_write_register(r, RX_PW_P0 + number, r->payload_size);
    RF24_write_register(r, EN_RXADDR,
        RF24_read_register(r, EN_RXADDR) | (1 << number));
}

void RF24_closeReadingPipe(RF24_t *r, uint8_t pipe) {
    if (pipe > 5) return;
    RF24_write_register(r, EN_RXADDR,
        RF24_read_register(r, EN_RXADDR) & ~(1 << pipe));
    if (pipe == 0) r->p0_rx_active = 0;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Configuration
 * ───────────────────────────────────────────────────────────────────────── */

void RF24_setChannel(RF24_t *r, uint8_t channel) {
    RF24_write_register(r, RF_CH, (channel < 125) ? channel : 125);
}

uint8_t RF24_getChannel(RF24_t *r) {
    return RF24_read_register(r, RF_CH);
}

void RF24_setPayloadSize(RF24_t *r, uint8_t size) {
    r->payload_size = (size > 32) ? 32 : ((size < 1) ? 1 : size);
}

uint8_t RF24_getPayloadSize(RF24_t *r) {
    return r->payload_size;
}

uint8_t RF24_getDynamicPayloadSize(RF24_t *r) {
    uint8_t result;
    _csn(r, 0);
    _spi_xfer(r, R_RX_PL_WID);
    result = _spi_xfer(r, RF24_NOP);
    _csn(r, 1);
    if (result > 32) { RF24_flush_rx(r); result = 0; }
    return result;
}

void RF24_setPALevel(RF24_t *r, rf24_pa_dbm_e level, uint8_t lnaEnable) {
    uint8_t setup = RF24_read_register(r, RF_SETUP) & 0xF8;
    uint8_t pwr;
    if      (level == RF24_PA_MAX)  pwr = 3;
    else if (level == RF24_PA_HIGH) pwr = 2;
    else if (level == RF24_PA_LOW)  pwr = 1;
    else                            pwr = 0;
    setup |= (pwr << RF_PWR_LOW) | (lnaEnable ? 1 : 0);
    RF24_write_register(r, RF_SETUP, setup);
}

rf24_pa_dbm_e RF24_getPALevel(RF24_t *r) {
    return (rf24_pa_dbm_e)((RF24_read_register(r, RF_SETUP) >> RF_PWR_LOW) & 0x03);
}

uint8_t RF24_setDataRate(RF24_t *r, rf24_datarate_e speed) {
    uint8_t setup = RF24_read_register(r, RF_SETUP);
    setup &= ~((1 << RF_DR_LOW) | (1 << RF_DR_HIGH));
    if      (speed == RF24_250KBPS) setup |= (1 << RF_DR_LOW);
    else if (speed == RF24_2MBPS)   setup |= (1 << RF_DR_HIGH);
    RF24_write_register(r, RF_SETUP, setup);
    return (RF24_read_register(r, RF_SETUP) == setup);
}

rf24_datarate_e RF24_getDataRate(RF24_t *r) {
    uint8_t setup = RF24_read_register(r, RF_SETUP);
    if (setup & (1 << RF_DR_LOW))  return RF24_250KBPS;
    if (setup & (1 << RF_DR_HIGH)) return RF24_2MBPS;
    return RF24_1MBPS;
}

void RF24_setCRCLength(RF24_t *r, rf24_crclength_e length) {
    r->config_reg &= ~((1 << EN_CRC) | (1 << CRCO));
    if      (length == RF24_CRC_8)  r->config_reg |= (1 << EN_CRC);
    else if (length == RF24_CRC_16) r->config_reg |= (1 << EN_CRC) | (1 << CRCO);
    RF24_write_register(r, NRF_CONFIG, r->config_reg);
}

rf24_crclength_e RF24_getCRCLength(RF24_t *r) {
    uint8_t cfg = RF24_read_register(r, NRF_CONFIG);
    if (!(cfg & (1 << EN_CRC))) return RF24_CRC_DISABLED;
    return (cfg & (1 << CRCO)) ? RF24_CRC_16 : RF24_CRC_8;
}

void RF24_disableCRC(RF24_t *r) {
    RF24_setCRCLength(r, RF24_CRC_DISABLED);
}

void RF24_setAutoAck(RF24_t *r, uint8_t enable) {
    RF24_write_register(r, EN_AA, enable ? 0x3F : 0x00);
}

void RF24_setAutoAckPipe(RF24_t *r, uint8_t pipe, uint8_t enable) {
    if (pipe > 5) return;
    uint8_t en_aa = RF24_read_register(r, EN_AA);
    if (enable) en_aa |=  (1 << pipe);
    else        en_aa &= ~(1 << pipe);
    RF24_write_register(r, EN_AA, en_aa);
}

void RF24_setRetries(RF24_t *r, uint8_t delay, uint8_t count) {
    RF24_write_register(r, SETUP_RETR,
        (uint8_t)((delay & 0x0F) << ARD) | (count & 0x0F));
}

void RF24_setAddressWidth(RF24_t *r, uint8_t width) {
    if (width < 3 || width > 5) return;
    r->addr_width = width;
    RF24_write_register(r, SETUP_AW, (width - 2) & 0x03);
}

void RF24_enableDynamicPayloads(RF24_t *r) {
    RF24_write_register(r, FEATURE,
        RF24_read_register(r, FEATURE) | (1 << EN_DPL));
    RF24_write_register(r, DYNPD, 0x3F);
    r->dynamic_payloads_enabled = 1;
}

void RF24_disableDynamicPayloads(RF24_t *r) {
    RF24_write_register(r, DYNPD, 0x00);
    RF24_write_register(r, FEATURE,
        RF24_read_register(r, FEATURE) & ~(1 << EN_DPL));
    r->dynamic_payloads_enabled = 0;
}

void RF24_enableAckPayload(RF24_t *r) {
    RF24_write_register(r, FEATURE,
        RF24_read_register(r, FEATURE) | (1 << EN_ACK_PAY) | (1 << EN_DPL));
    RF24_write_register(r, DYNPD,
        RF24_read_register(r, DYNPD) | (1 << DPL_P0) | (1 << DPL_P1));
    r->ack_payloads_enabled     = 1;
    r->dynamic_payloads_enabled = 1;
}

void RF24_disableAckPayload(RF24_t *r) {
    RF24_write_register(r, FEATURE,
        RF24_read_register(r, FEATURE) &
        ~((1 << EN_ACK_PAY) | (1 << EN_DPL)));
    RF24_write_register(r, DYNPD, 0x00);
    r->ack_payloads_enabled     = 0;
    r->dynamic_payloads_enabled = 0;
}

void RF24_enableDynamicAck(RF24_t *r) {
    RF24_write_register(r, FEATURE,
        RF24_read_register(r, FEATURE) | (1 << EN_DYN_ACK));
}

uint8_t RF24_writeAckPayload(RF24_t *r, uint8_t pipe,
                             const void *buf, uint8_t len) {
    if (!r->ack_payloads_enabled) return 0;
    if (RF24_read_register(r, FIFO_STATUS) & (1 << FIFO_FULL)) return 0;

    _csn(r, 0);
    r->status = _spi_xfer(r, W_ACK_PAYLOAD | (pipe & 0x07));
    const uint8_t *p = (const uint8_t *)buf;
    uint8_t data_len = (len > 32) ? 32 : len;
    for (uint8_t i = 0; i < data_len; i++) { _spi_xfer(r, p[i]); }
    _csn(r, 1);
    return 1;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Power management
 * ───────────────────────────────────────────────────────────────────────── */

void RF24_powerUp(RF24_t *r) {
    if (!(r->config_reg & (1 << PWR_UP))) {
        r->config_reg |= (1 << PWR_UP);
        RF24_write_register(r, NRF_CONFIG, r->config_reg);
        HAL_Delay(5);
    }
}

void RF24_powerDown(RF24_t *r) {
    _ce(r, 0);
    r->config_reg &= ~(1 << PWR_UP);
    RF24_write_register(r, NRF_CONFIG, r->config_reg);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Diagnostics
 * ───────────────────────────────────────────────────────────────────────── */

uint8_t RF24_isChipConnected(RF24_t *r) {
    uint8_t setup = RF24_read_register(r, SETUP_AW);
    return (setup >= 1 && setup <= 3);
}

uint8_t RF24_isPVariant(RF24_t *r) {
    return r->is_p_variant;
}

uint8_t RF24_testRPD(RF24_t *r) {
    return (RF24_read_register(r, RPD) & 1);
}

uint8_t RF24_testCarrier(RF24_t *r) {
    return (RF24_read_register(r, CD) & 1);
}

uint8_t RF24_flush_tx(RF24_t *r) {
    uint8_t status;
    _csn(r, 0);
    status = _spi_xfer(r, FLUSH_TX);
    _csn(r, 1);
    return status;
}

uint8_t RF24_flush_rx(RF24_t *r) {
    uint8_t status;
    _csn(r, 0);
    status = _spi_xfer(r, FLUSH_RX);
    _csn(r, 1);
    return status;
}

uint8_t RF24_getStatus(RF24_t *r) {
    uint8_t status;
    _csn(r, 0);
    status = _spi_xfer(r, RF24_NOP);
    _csn(r, 1);
    r->status = status;
    return status;
}
