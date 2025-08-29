#include "LoraController.h"

// RX/TX buffers (RX only used for debug)
char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

// ---- TX queue ----
struct TxItem {
  uint8_t buf[BUFFER_SIZE];
  uint8_t len;
};
static constexpr uint8_t TXQ_CAP = 4;
static TxItem txq[TXQ_CAP];
static uint8_t txq_head = 0, txq_tail = 0, txq_count = 0;
static bool txInProgress = false;

int16_t Rssi = 0, rxSize = 0;

static void OnTxDone(void);
static void OnTxTimeout(void);
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

// Enqueue a packet (non-blocking). Returns false if full or too long.
static bool enqueueTx(const uint8_t* data, size_t len) {
  if (len == 0 || len > BUFFER_SIZE) return false;
  if (txq_count >= TXQ_CAP) return false;
  TxItem &slot = txq[ (txq_tail) % TXQ_CAP ];
  memcpy(slot.buf, data, len);
  slot.len = static_cast<uint8_t>(len);
  txq_tail = (txq_tail + 1) % TXQ_CAP;
  txq_count++;
  return true;
}

bool radioHasPendingTx() {
  return txInProgress || (txq_count > 0);
}

bool radioAwaitTxDone(uint32_t timeout_ms) {
  uint32_t t0 = millis();
  while (radioHasPendingTx() && (millis() - t0 < timeout_ms)) {
    radioService();   // runs IrqProcess + pumpTx
    delay(1);
  }
  return !radioHasPendingTx();
}

// If idle and queue not empty, start a TX.
static void pumpTx() {
  if (txInProgress || txq_count == 0) return;

  TxItem &item = txq[ txq_head ];
  txInProgress = true;

  Radio.Standby();
  // Debug print
  Serial.printf("Sending package:");
  for (uint8_t i = 0; i < item.len; i++) Serial.printf(" 0x%02X", item.buf[i]);
  Serial.println();

  Radio.Send(const_cast<uint8_t*>(item.buf), item.len);
  // Do NOT force Rx here; OnTxDone/OnTxTimeout decides what to do next
}

// Public: safe to call from anywhere, even inside RX paths.
bool sendMessage(const uint8_t* data, size_t len) {
  return enqueueTx(data, len);
}

// Call this often in loop(): runs IRQs and starts any pending TX
void radioService() {
  Radio.IrqProcess();
  pumpTx();
}

// Init
void radioInit() {
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.TxDone    = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone    = OnRxDone;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  Serial.println("___________RX start___________");
  Radio.Rx(0);
}

// ---- Callbacks ----
static void OnTxDone(void) {
  // consume the item we just sent
  if (txq_count) {
    txq_head = (txq_head + 1) % TXQ_CAP;
    txq_count--;
  }
  txInProgress = false;

  Serial.println(">>> Message sent <<<");

  // If more to send, start next immediately; else back to RX.
  if (txq_count > 0) {
    pumpTx();
  } else {
    Radio.Rx(0);
    Serial.println("___________RX resumed___________");
  }
}

static void OnTxTimeout(void) {
  // drop the attempted item (same as TxDone)
  if (txq_count) {
    txq_head = (txq_head + 1) % TXQ_CAP;
    txq_count--;
  }
  txInProgress = false;

  Radio.Sleep();
  Serial.println("[WARNING] Message timed out!");

  if (txq_count > 0) {
    pumpTx();
  } else {
    Radio.Rx(0);
    Serial.println("___________RX resumed___________");
  }
}

static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  Rssi = rssi;
  rxSize = size;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  // Put radio to sleep while we process
  Radio.Sleep();

  extern void processReceivedPacket(const uint8_t* data, uint16_t len, int16_t rssi, int8_t snr);
  processReceivedPacket(payload, size, rssi, snr);

  // If a TX was queued during processing, pump it; else go back to RX
  if (txq_count > 0 && !txInProgress) {
    pumpTx();
  } else {
    Radio.Rx(0);
    Serial.println("___________RX resumed___________");
  }
}
