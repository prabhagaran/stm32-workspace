/*
 * nRF24L01.h - Register map and instruction/bit definitions for nRF24L01(+)
 *
 * Original: Stefan Engelke / Greg Copeland / nRF24 project
 * This file is a direct, clean copy of the register constants — no Arduino deps.
 *
 * MIT License — see original nRF24/RF24 repo for full attribution.
 */

#ifndef NRF24L01_H_
#define NRF24L01_H_

/* ===== Register map ===== */
#define NRF_CONFIG      0x00  /* Configuration Register         */
#define EN_AA           0x01  /* Enable Auto Acknowledgment     */
#define EN_RXADDR       0x02  /* Enabled RX Addresses           */
#define SETUP_AW        0x03  /* Setup of Address Width         */
#define SETUP_RETR      0x04  /* Setup of Automatic Retransmit  */
#define RF_CH           0x05  /* RF Channel                     */
#define RF_SETUP        0x06  /* RF Setup Register              */
#define NRF_STATUS      0x07  /* Status Register                */
#define OBSERVE_TX      0x08  /* Transmit Observe Register      */
#define CD              0x09  /* Carrier Detect (nRF24L01)      */
#define RPD             0x09  /* Received Power Detector (nRF24L01+) */
#define RX_ADDR_P0      0x0A  /* Receive Address Pipe 0         */
#define RX_ADDR_P1      0x0B  /* Receive Address Pipe 1         */
#define RX_ADDR_P2      0x0C  /* Receive Address Pipe 2         */
#define RX_ADDR_P3      0x0D  /* Receive Address Pipe 3         */
#define RX_ADDR_P4      0x0E  /* Receive Address Pipe 4         */
#define RX_ADDR_P5      0x0F  /* Receive Address Pipe 5         */
#define TX_ADDR         0x10  /* Transmit Address               */
#define RX_PW_P0        0x11  /* RX Payload width, pipe 0       */
#define RX_PW_P1        0x12  /* RX Payload width, pipe 1       */
#define RX_PW_P2        0x13  /* RX Payload width, pipe 2       */
#define RX_PW_P3        0x14  /* RX Payload width, pipe 3       */
#define RX_PW_P4        0x15  /* RX Payload width, pipe 4       */
#define RX_PW_P5        0x16  /* RX Payload width, pipe 5       */
#define FIFO_STATUS     0x17  /* FIFO Status Register           */
#define DYNPD           0x1C  /* Enable dynamic payload length  */
#define FEATURE         0x1D  /* Feature Register               */

/* ===== NRF_CONFIG register bits ===== */
#define MASK_RX_DR      6   /* Mask interrupt caused by RX_DR  */
#define MASK_TX_DS      5   /* Mask interrupt caused by TX_DS  */
#define MASK_MAX_RT     4   /* Mask interrupt caused by MAX_RT */
#define EN_CRC          3   /* Enable CRC                      */
#define CRCO            2   /* CRC encoding scheme (0=1byte, 1=2bytes) */
#define PWR_UP          1   /* Power Up                        */
#define PRIM_RX         0   /* RX/TX control (1=PRX, 0=PTX)   */

/* ===== EN_AA bits ===== */
#define ENAA_P5         5
#define ENAA_P4         4
#define ENAA_P3         3
#define ENAA_P2         2
#define ENAA_P1         1
#define ENAA_P0         0

/* ===== EN_RXADDR bits ===== */
#define ERX_P5          5
#define ERX_P4          4
#define ERX_P3          3
#define ERX_P2          2
#define ERX_P1          1
#define ERX_P0          0

/* ===== SETUP_AW bits ===== */
#define AW              0   /* Address width (2 bits): 01=3, 10=4, 11=5 */

/* ===== SETUP_RETR bits ===== */
#define ARD             4   /* Auto Retransmit Delay (4 bits) */
#define ARC             0   /* Auto Retransmit Count (4 bits) */

/* ===== RF_SETUP bits ===== */
#define CONT_WAVE       7   /* Enable continuous carrier transmit */
#define RF_DR_LOW       5   /* Set 250kbps (requires nRF24L01+)   */
#define PLL_LOCK        4   /* Force PLL lock signal              */
#define RF_DR_HIGH      3   /* Select high speed data rate (2Mbps) when RF_DR_LOW=0 */
#define RF_PWR          1   /* Set RF output power (2 bits)       */
#define RF_DR           3   /* (legacy, same as RF_DR_HIGH)       */

/* ===== NRF_STATUS bits ===== */
#define RX_DR           6   /* Data Ready RX FIFO interrupt       */
#define TX_DS           5   /* Data Sent TX FIFO interrupt        */
#define MAX_RT          4   /* Maximum number of TX retransmissions */
#define RX_P_NO         1   /* Data pipe number (3 bits)          */
#define TX_FULL         0   /* TX FIFO full flag                  */

/* ===== OBSERVE_TX bits ===== */
#define PLOS_CNT        4   /* Count lost packets (4 bits)        */
#define ARC_CNT         0   /* Count retransmitted packets (4 bits) */

/* ===== FIFO_STATUS bits ===== */
#define TX_REUSE        6
#define FIFO_FULL       5
#define TX_EMPTY        4
#define RX_FULL         1
#define RX_EMPTY        0

/* ===== DYNPD bits ===== */
#define DPL_P5          5
#define DPL_P4          4
#define DPL_P3          3
#define DPL_P2          2
#define DPL_P1          1
#define DPL_P0          0

/* ===== FEATURE bits ===== */
#define EN_DPL          2   /* Enable dynamic payload length      */
#define EN_ACK_PAY      1   /* Enable payload with ACK            */
#define EN_DYN_ACK      0   /* Enable W_TX_PAYLOAD_NOACK command  */

/* ===== SPI Commands ===== */
#define R_REGISTER          0x00  /* Read register (OR with reg addr) */
#define W_REGISTER          0x20  /* Write register (OR with reg addr) */
#define REGISTER_MASK       0x1F  /* Mask for register address        */
#define ACTIVATE            0x50  /* Activate features (nRF24L01)     */
#define R_RX_PL_WID         0x60  /* Read RX payload width            */
#define R_RX_PAYLOAD        0x61  /* Read RX payload                  */
#define W_TX_PAYLOAD        0xA0  /* Write TX payload                 */
#define W_ACK_PAYLOAD       0xA8  /* Write ACK payload (OR with pipe) */
#define W_TX_PAYLOAD_NO_ACK 0xB0  /* Write TX payload, no ACK         */
#define FLUSH_TX            0xE1  /* Flush TX FIFO                    */
#define FLUSH_RX            0xE2  /* Flush RX FIFO                    */
#define REUSE_TX_PL         0xE3  /* Reuse last transmitted payload   */
#define RF24_NOP            0xFF  /* No Operation (get status)        */

/* ===== LNA (Low Noise Amplifier) ===== */
#define LNA_HCURR           0     /* nRF24L01 non-plus omission       */

/* ===== RF_PWR levels (bits [2:1] of RF_SETUP) ===== */
#define RF_PWR_LOW          1
#define RF_PWR_HIGH         2

#endif /* NRF24L01_H_ */
