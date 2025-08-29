#pragma once
#include "LoRaWan_APP.h"
#include <stdint.h>
#include <string.h>

#define RF_FREQUENCY   868100000 
#define TX_OUTPUT_POWER 5
#define LORA_BANDWIDTH  0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 30

// ---------- Public API you can call from anywhere ----------
void radioInit();

bool sendMessage(const uint8_t* data, size_t len = 1);

void radioService();

bool radioHasPendingTx();

bool radioAwaitTxDone(uint32_t timeout_ms = 600);

// -----------------------------------------------------------

extern int16_t Rssi;
extern int16_t rxSize;