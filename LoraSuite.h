// LoRaHal.ino — Minimal blocking send/receive helpers for Heltec SX126x
#include "Arduino.h"
#include "LoRaWan_APP.h"

// ---------- Radio params (edit to taste) ----------
#ifndef LORA_RF_FREQUENCY
#define LORA_RF_FREQUENCY               868000000UL // Hz
#endif
#ifndef LORA_BANDWIDTH
#define LORA_BANDWIDTH                  0           // 0:125k, 1:250k, 2:500k
#endif
#ifndef LORA_SPREADING_FACTOR
#define LORA_SPREADING_FACTOR           7           // SF7..SF12
#endif
#ifndef LORA_CODINGRATE
#define LORA_CODINGRATE                 1           // 1:4/5, 2:4/6, 3:4/7, 4:4/8
#endif
#ifndef LORA_PREAMBLE_LENGTH
#define LORA_PREAMBLE_LENGTH            8
#endif
#ifndef LORA_TX_POWER_DBM
#define LORA_TX_POWER_DBM               5           // dBm
#endif
#ifndef LORA_FIX_LENGTH_PAYLOAD_ON
#define LORA_FIX_LENGTH_PAYLOAD_ON      false
#endif
#ifndef LORA_IQ_INVERSION_ON
#define LORA_IQ_INVERSION_ON            false
#endif

// ---------- Internal state ----------
static RadioEvents_t _evt;
static volatile bool _txDone   = false;
static volatile bool _txError  = false;
static volatile bool _rxDone   = false;
static volatile bool _rxError  = false;
static volatile uint16_t _rxSize = 0;
static volatile int16_t  _rxRssi = 0;
static volatile int8_t   _rxSnr  = 0;
static bool _loraReady = false;

// A small internal RX buffer; user provides their own buffer to copy into.
#ifndef LORA_INTERNAL_RXBUF_SIZE
#define LORA_INTERNAL_RXBUF_SIZE 256
#endif
static uint8_t _rxBuf[LORA_INTERNAL_RXBUF_SIZE];

// ---------- Callbacks ----------
static void _OnTxDone(void)                 { _txDone = true; }
static void _OnTxTimeout(void)              { _txError = true; }
static void _OnRxTimeout(void)              { _rxError = true; }
static void _OnRxError(void)                { _rxError = true; }
static void _OnRxDone(uint8_t *pl, uint16_t size, int16_t rssi, int8_t snr) {
  uint16_t n = (size < LORA_INTERNAL_RXBUF_SIZE) ? size : (LORA_INTERNAL_RXBUF_SIZE - 1);
  memcpy(_rxBuf, pl, n);
  _rxBuf[n] = 0;
  _rxSize = n;
  _rxRssi = rssi;
  _rxSnr  = snr;
  _rxDone = true;
}

// ---------- Init (lazy) ----------
static void _ensureInit() {
  if (_loraReady) return;

  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  _evt.TxDone    = _OnTxDone;
  _evt.TxTimeout = _OnTxTimeout;
  _evt.RxDone    = _OnRxDone;
  _evt.RxTimeout = _OnRxTimeout;
  _evt.RxError   = _OnRxError;

  Radio.Init(&_evt);
  Radio.SetChannel(LORA_RF_FREQUENCY);

  Radio.SetTxConfig(
    MODEM_LORA,
    LORA_TX_POWER_DBM, 0, LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
    true, 0, 0, LORA_IQ_INVERSION_ON, 3000
  );

  Radio.SetRxConfig(
    MODEM_LORA,
    LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE,
    0, LORA_PREAMBLE_LENGTH, 0,
    LORA_FIX_LENGTH_PAYLOAD_ON, 0, true, 0, 0, LORA_IQ_INVERSION_ON,
    false // not continuous; we manage timeout per call
  );

  _loraReady = true;
  Serial.println("LORA RECEIVER INITIALISED"); 
}

// ---------- Public API ----------
void loraSend(const uint8_t* data, uint16_t len, uint32_t txTimeoutMs /*=3000*/) {
  _ensureInit();
  _txDone = false;
  _txError = false;

  // Fire TX
  Radio.Send((uint8_t*)data, len);

  // Wait for TX complete or timeout
  uint32_t deadline = millis() + (txTimeoutMs ? txTimeoutMs : 3000);
  while (!_txDone && !_txError) {
    Radio.IrqProcess();
    if ((int32_t)(millis() - deadline) >= 0) {
      // If radio didn't call TxTimeout, treat as timeout
      _txError = true;
      break;
    }
    delay(1);
  }

  // Make sure radio isn't stuck in TX state
  Radio.Sleep();
  // --- Report status ---
  if (_txDone && !_txError) {
    Serial.println("LORA SEND SUCCESS");
  } else {
    Serial.println("LORA SEND FAILED");
  }
}

int loraReceive(uint8_t* out, uint16_t outMax, int16_t* rssi, int8_t* snr, uint32_t rxTimeoutMs) {
  _ensureInit();
  _rxDone = false;
  _rxError = false;
  _rxSize = 0;

  Serial.println("LORA RECEIVER: Listening for messages");

  // Start RX with timeout (0 => wait forever)
  Radio.Rx(rxTimeoutMs);

  // Wait for RX complete or timeout/error
  // Note: RX timeout triggers _rxError via callback.
  uint32_t deadline = millis() + (rxTimeoutMs ? rxTimeoutMs : 0xFFFFFFFF);
  while (!_rxDone && !_rxError) {
    Radio.IrqProcess();
    if (rxTimeoutMs && (int32_t)(millis() - deadline) >= 0) {
      // Safety net if callback didn't fire
      _rxError = true;
      break;
    }
    delay(1);
  }

  // Stop the radio to save power
  Radio.Sleep();
  Serial.println("LORA SLEEP ON");
  if (!_rxDone || _rxSize == 0) return 0;

  // Copy out
  uint16_t n = (_rxSize < outMax) ? _rxSize : outMax;
  memcpy(out, _rxBuf, n);
  if (rssi) *rssi = _rxRssi;
  if (snr)  *snr  = _rxSnr;
  return n;
}
