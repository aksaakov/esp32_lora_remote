#include "LoRaWan_APP.h"

#define RF_FREQUENCY                                865000000
#define TX_OUTPUT_POWER                             5
#define LORA_BANDWIDTH                              0
#define LORA_SPREADING_FACTOR                       7
#define LORA_CODINGRATE                             1
#define LORA_PREAMBLE_LENGTH                        8
#define LORA_SYMBOL_TIMEOUT                         0
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false
#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30

#pragma once

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void processReceivedPacket(const uint8_t* data);
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void processReceivedPacket(const uint8_t* data, uint16_t len, int16_t rssi, int8_t snr);

typedef enum {
    LOWPOWER,
    STATE_RX,
    STATE_TX
  } States_t;


States_t state;
bool sleepMode = false;
int16_t Rssi,rxSize;

static void radioInit() {
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
    Rssi=0;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxDone = OnRxDone;

    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    Serial.println("___________RX start___________");                                    
    Radio.Rx(0);
}

void OnTxDone( void )
{
  Serial.println(">>> Message sent <<<");  
  Radio.Rx(0);
  Serial.println("___________RX resumed___________"); 
}

void OnTxTimeout( void )
{
    Radio.Sleep();
    Serial.println("[WARNING] Message timed out!"); 
    Radio.Rx(0);
    Serial.println("___________RX resumed___________");
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Rssi=rssi;
    rxSize=size;
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    Radio.Sleep();

    
    processReceivedPacket(payload, size, rssi, snr);
    // Serial.println(">>> Message received <<<"); 
    Radio.Rx(0);
    Serial.println("___________RX resumed___________");
}

