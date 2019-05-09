/******************************************************************************
  * @file    lora.c
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    10-July-2018
  * @brief   lora API to drive the lora state Machine
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hw.h"
#include "timeServer.h"
#include "LoRaMac.h"
#include "lora.h"
#include "lora-test.h"
#include "tiny_sscanf.h"
#include "at.h"
#include "eeprom.h"

 /**
   * Lora Configuration
   */
 typedef struct
 {
   LoraState_t otaa;        /*< ENABLE if over the air activation, DISABLE otherwise */
   LoraState_t duty_cycle;  /*< ENABLE if dutycyle is on, DISABLE otherwise */
   uint8_t DevEui[8];           /*< Device EUI */
   uint8_t JoinEui[8];           /*< Join Eui */
   uint8_t AppKey[16];          /*< Application Key */
   uint8_t NwkKey[16];          /*< Application Key */
   uint8_t NwkSEncKey[16];         /*< Network Session Key */
   uint8_t AppSKey[16];         /*< Application Session Key */
   uint8_t FNwkSIntKey[16];         /*< Application Session Key */
   uint8_t SNwkSIntKey[16];         /*< Application Session Key */
   int16_t Rssi;                /*< Rssi of the received packet */
   uint8_t Snr;                 /*< Snr of the received packet */
   uint8_t application_port;    /*< Application port we will receive to */
   LoraConfirm_t ReqAck;      /*< ENABLE if acknowledge is requested */
   McpsConfirm_t *McpsConfirm;  /*< pointer to the confirm structure */
   int8_t TxDatarate;
   int8_t TxPower;
   uint32_t Rx2Frequency;
   uint32_t Rx2DataRate;
   uint32_t Rx1Delay;
   uint32_t Rx2Delay;
   uint32_t JoinDelay1;
   uint32_t JoinDelay2;
   uint32_t netid;
   uint32_t class;
   uint32_t subBand;
 } lora_configuration_t;


static lora_configuration_t lora_config =
{
  .otaa = LORA_DISABLE,
#if defined( REGION_EU868 )
  .duty_cycle = LORA_ENABLE,
#else
  .duty_cycle = LORA_DISABLE,
#endif
  .DevEui = LORAWAN_DEVICE_EUI,
  .JoinEui = LORAWAN_JOIN_EUI,
  .AppKey = LORAWAN_APP_KEY,
  .NwkKey = LORAWAN_NWK_KEY,
  .NwkSEncKey = LORAWAN_NWK_S_ENC_KEY,
  .AppSKey = LORAWAN_APP_S_KEY,
  .FNwkSIntKey = LORAWAN_F_NWK_S_INT_KEY,
  .SNwkSIntKey = LORAWAN_S_NWK_S_INT_KEY,
  .Rssi = 0,
  .Snr = 0,
  .ReqAck = LORAWAN_UNCONFIRMED_MSG,
  .McpsConfirm = NULL,
  .TxDatarate = 0,
  .TxPower = 0,
  .Rx2Frequency = 923300000,
  .Rx2DataRate = 8,
  .Rx1Delay = 1000,
  .Rx2Delay = 2000,
  .JoinDelay1 = 5000,
  .JoinDelay2 = 6000,
  .netid = 0,
  .class = 0x00,
  .subBand = 0x01
};

/*!
 *  Select either Device_Time_req or Beacon_Time_Req following LoRaWAN version
 *  - Device_Time_req   Available for V1.0.3 or later
 *  - Beacon_time_Req   Available for V1.0.2 and before
 */
/*#define USE_DEVICE_TIMING*/

/*!
 * Join requests trials duty cycle.
 */
#define OVER_THE_AIR_ACTIVATION_DUTYCYCLE           10000  // 10 [s] value in ms

#include "LoRaMacTest.h"

#if defined( REGION_EU868 )
/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */
#define LORAWAN_DUTYCYCLE_ON                        true

#endif
/*!
 * Default ping slots periodicity
 *
 * \remark periodicity is equal to 2^LORAWAN_DEFAULT_PING_SLOT_PERIODICITY seconds
 *         example: 2^3 = 8 seconds. The end-device will open an Rx slot every 8 seconds.
 */
#define LORAWAN_DEFAULT_PING_SLOT_PERIODICITY       0

#define HEX16(X)  X[0],X[1], X[2],X[3], X[4],X[5], X[6],X[7],X[8],X[9], X[10],X[11], X[12],X[13], X[14],X[15]
#define HEX8(X)   X[0],X[1], X[2],X[3], X[4],X[5], X[6],X[7]

static MlmeReqJoin_t JoinParameters;

static uint32_t DevAddr = LORAWAN_DEVICE_ADDRESS;
uint32_t FLAG_CODE = 9957;

static LoraErrorStatus LORA_BeaconReq( void);
static LoraErrorStatus LORA_PingSlotReq( void);

#if defined( USE_DEVICE_TIMING )
static LoraErrorStatus LORA_DeviceTimeReq(void);
#else
static LoraErrorStatus LORA_BeaconTimeReq(void);
#endif

/*!
 * Defines the LoRa parameters at Init
 */
static LoRaParam_t* LoRaParamInit;
static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacCallback_t LoRaMacCallbacks;
static MibRequestConfirm_t mibReq;

static LoRaMainCallback_t *LoRaMainCallbacks;
/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] McpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void McpsConfirm( McpsConfirm_t *mcpsConfirm )
{
    if( mcpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
    {
        switch( mcpsConfirm->McpsRequest )
        {
            case MCPS_UNCONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                break;
            }
            case MCPS_CONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                // Check AckReceived
                // Check NbTrials
                break;
            }
            case MCPS_PROPRIETARY:
            {
                break;
            }
            default:
                break;
        }
    }
}

/*!
 * \brief   MCPS-Indication event function
 *
 * \param   [IN] mcpsIndication - Pointer to the indication structure,
 *               containing indication attributes.
 */
static void McpsIndication( McpsIndication_t *mcpsIndication )
{
  lora_AppData_t AppData;

    if( mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK )
    {
        return;
    }

    switch( mcpsIndication->McpsIndication )
    {
        case MCPS_UNCONFIRMED:
        {
            break;
        }
        case MCPS_CONFIRMED:
        {
            break;
        }
        case MCPS_PROPRIETARY:
        {
            break;
        }
        case MCPS_MULTICAST:
        {
            break;
        }
        default:
            break;
    }

    // Check Multicast
    // Check Port
    // Check Datarate
    // Check FramePending
    if( mcpsIndication->FramePending == true )
    {
        // The server signals that it has pending data to be sent.
        // We schedule an uplink as soon as possible to flush the server.
        LoRaMainCallbacks->LORA_TxNeeded( );
    }
    // Check Buffer
    // Check BufferSize
    // Check Rssi
    // Check Snr
    // Check RxSlot
    if (certif_running() == true )
    {
      certif_DownLinkIncrement( );
    }

    if( mcpsIndication->RxData == true )
    {
      switch( mcpsIndication->Port )
      {
        case CERTIF_PORT:
          certif_rx( mcpsIndication, &JoinParameters );
          break;
        default:

          AppData.Port = mcpsIndication->Port;
          AppData.BuffSize = mcpsIndication->BufferSize;
          AppData.Buff = mcpsIndication->Buffer;
          lora_config.Rssi = mcpsIndication->Rssi;
          lora_config.Snr  = mcpsIndication->Snr;
          LoRaMainCallbacks->LORA_RxData( &AppData );
          break;
      }
    }
}

/*!
 * \brief   MLME-Confirm event function
 *
 * \param   [IN] MlmeConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void MlmeConfirm( MlmeConfirm_t *mlmeConfirm )
{
    MibRequestConfirm_t mibReq;

    switch( mlmeConfirm->MlmeRequest )
    {
        case MLME_JOIN:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
              // Status is OK, node has joined the network
              LoRaMainCallbacks->LORA_HasJoined();
#if defined( USE_DEVICE_TIMING )
              LORA_DeviceTimeReq();
#else
              LORA_BeaconTimeReq();
#endif
            }
            else
            {
                // Join was not successful. Try to join again
                LORA_Join();
            }
            break;
        }
        case MLME_LINK_CHECK:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                // Check DemodMargin
                // Check NbGateways
                if (certif_running() == true )
                {
                     certif_linkCheck( mlmeConfirm);
                }
            }
            break;
        }
        case MLME_BEACON_ACQUISITION:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                /* Beacon has been acquired */
                /* REquest Server for Ping Slot */
                LORA_PingSlotReq( );
            }
            else
            {
                /* Beacon not acquired */
                /* Search again */
                /* we can check if the MAC has received a time reference for the beacon*/
                /* in this case do either a Device_Time_Req  or a Beacon_Timing_req*/
                LORA_BeaconReq( );
            }
            break;
        }
        case MLME_PING_SLOT_INFO:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
               /* class B is now ativated*/
                mibReq.Type = MIB_DEVICE_CLASS;
                mibReq.Param.Class = CLASS_B;
                LoRaMacMibSetRequestConfirm( &mibReq );

#if defined( REGION_AU915 ) || defined( REGION_US915 ) || defined( REGION_US915_HYBRID )
                mibReq.Type = MIB_PING_SLOT_DATARATE;
                mibReq.Param.PingSlotDatarate = DR_8;
                LoRaMacMibSetRequestConfirm( &mibReq );
#endif

                /*notify upper layer*/
                LoRaMainCallbacks->LORA_ConfirmClass(CLASS_B);
            }
            else
            {
                LORA_PingSlotReq( );
            }
            break;
        }
#if defined( USE_DEVICE_TIMING )
        case MLME_DEVICE_TIME:
        {
            if( mlmeConfirm->Status != LORAMAC_EVENT_INFO_STATUS_OK )
            {
              LORA_DeviceTimeReq();
            }
        }
#endif
        default:
            break;
    }
}

/*!
 * \brief   MLME-Indication event function
 *
 * \param   [IN] MlmeIndication - Pointer to the indication structure.
 */
static void MlmeIndication( MlmeIndication_t *MlmeIndication )
{
    MibRequestConfirm_t mibReq;


    switch( MlmeIndication->MlmeIndication )
    {
        case MLME_SCHEDULE_UPLINK:
        {
            // The MAC signals that we shall provide an uplink as soon as possible
            LoRaMainCallbacks->LORA_TxNeeded( );
            break;
        }
        case MLME_BEACON_LOST:
        {
            // Switch to class A again
            mibReq.Type = MIB_DEVICE_CLASS;
            mibReq.Param.Class = CLASS_A;
            LoRaMacMibSetRequestConfirm( &mibReq );

            LORA_BeaconReq();
            break;
        }
        case MLME_BEACON:
        {
            if( MlmeIndication->Status == LORAMAC_EVENT_INFO_STATUS_BEACON_LOCKED )
            {
              TVL2( PRINTF( "BEACON RECEIVED\n\r");)
            }
            else
            {
              TVL2( PRINTF( "BEACON NOT RECEIVED\n\r");)
            }
            break;

        }
        default:
            break;
    }
}

void update_flag() {
  uint32_t flag = 0;

  // FLAG from memory
  flag = readEEPROMByte(FLAG_DIFF);
  if(flag == FLAG_CODE) {
      FLAG_CODE = readEEPROMByte(FLAG_ADDRESS);
  }
}

void get_params_from_memory() {
  uint32_t flag = 0;

  // DEV EUI
  flag = readEEPROMByte(DEV_EUI_FLAG);
  if(flag == FLAG_CODE) {
      uint8_t deveui[8] = {0};
      uint32_t read_from_mem1 = readEEPROMByte(DEV_EUI_0);
      uint32_t read_from_mem2 = readEEPROMByte(DEV_EUI_1);

      // convert uint32_t to uint8_t
      deveui[0] = read_from_mem1 >> 24; deveui[1] = read_from_mem1 >> 16; deveui[2] = read_from_mem1 >> 8; deveui[3] = read_from_mem1;
      deveui[4] = read_from_mem2 >> 24; deveui[5] = read_from_mem2 >> 16; deveui[6] = read_from_mem2 >> 8; deveui[7] = read_from_mem2;

      // PPRINTF("DevEui from memory = %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX8(deveui));
      lora_config_deveui_set(deveui);
  }

  // APP EUI
  flag = readEEPROMByte(JOIN_EUI_FLAG);
  if(flag == FLAG_CODE) {
      uint8_t appeui[8] = {0};
      uint32_t read_from_mem1 = readEEPROMByte(JOIN_EUI_0);
      uint32_t read_from_mem2 = readEEPROMByte(JOIN_EUI_1);

      // convert uint32_t to uint8_t
      appeui[0] = read_from_mem1 >> 24; appeui[1] = read_from_mem1 >> 16; appeui[2] = read_from_mem1 >> 8; appeui[3] = read_from_mem1;
      appeui[4] = read_from_mem2 >> 24; appeui[5] = read_from_mem2 >> 16; appeui[6] = read_from_mem2 >> 8; appeui[7] = read_from_mem2;

      // PPRINTF("appeui from memory = %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX8(appeui));
      lora_config_joineui_set(appeui);
  }

  // Device Address
  flag = readEEPROMByte(DEV_ADR_FLAG);
  if(flag == FLAG_CODE) {
      DevAddr = readEEPROMByte(DEV_ADR);
  }

  // APP KEY
  flag = readEEPROMByte(APP_KEY_FLAG);
  if(flag == FLAG_CODE) {
      uint8_t appkey[16] = {0};
      uint32_t read_from_mem1 = readEEPROMByte(APP_KEY_0);
      uint32_t read_from_mem2 = readEEPROMByte(APP_KEY_1);
      uint32_t read_from_mem3 = readEEPROMByte(APP_KEY_2);
      uint32_t read_from_mem4 = readEEPROMByte(APP_KEY_3);

      // convert uint32_t to uint8_t
      appkey[0] = read_from_mem1 >> 24; appkey[1] = read_from_mem1 >> 16; appkey[2] = read_from_mem1 >> 8; appkey[3] = read_from_mem1;
      appkey[4] = read_from_mem2 >> 24; appkey[5] = read_from_mem2 >> 16; appkey[6] = read_from_mem2 >> 8; appkey[7] = read_from_mem2;
      appkey[8] = read_from_mem3 >> 24; appkey[9] = read_from_mem3 >> 16; appkey[10] = read_from_mem3 >> 8; appkey[11] = read_from_mem3;
      appkey[12] = read_from_mem4 >> 24; appkey[13] = read_from_mem4 >> 16; appkey[14] = read_from_mem4 >> 8; appkey[15] = read_from_mem4;

      // PPRINTF("appkey from memory = %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX16(appkey));
      lora_config_appkey_set(appkey);
  }

  // Network Section Key
  flag = readEEPROMByte(NWK_SKEY_FLAG);
  if(flag == FLAG_CODE) {
      uint8_t nwkskey[16] = {0};
      uint32_t read_from_mem1 = readEEPROMByte(NWK_SKEY_0);
      uint32_t read_from_mem2 = readEEPROMByte(NWK_SKEY_1);
      uint32_t read_from_mem3 = readEEPROMByte(NWK_SKEY_2);
      uint32_t read_from_mem4 = readEEPROMByte(NWK_SKEY_3);

      // convert uint32_t to uint8_t
      nwkskey[0] = read_from_mem1 >> 24; nwkskey[1] = read_from_mem1 >> 16; nwkskey[2] = read_from_mem1 >> 8; nwkskey[3] = read_from_mem1;
      nwkskey[4] = read_from_mem2 >> 24; nwkskey[5] = read_from_mem2 >> 16; nwkskey[6] = read_from_mem2 >> 8; nwkskey[7] = read_from_mem2;
      nwkskey[8] = read_from_mem3 >> 24; nwkskey[9] = read_from_mem3 >> 16; nwkskey[10] = read_from_mem3 >> 8; nwkskey[11] = read_from_mem3;
      nwkskey[12] = read_from_mem4 >> 24; nwkskey[13] = read_from_mem4 >> 16; nwkskey[14] = read_from_mem4 >> 8; nwkskey[15] = read_from_mem4;

      // PPRINTF("nwkskey from memory = %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX16(nwkskey));
      memcpy1(lora_config.NwkSEncKey, nwkskey, sizeof(lora_config.NwkSEncKey));
      memcpy1(lora_config.FNwkSIntKey, nwkskey, sizeof(lora_config.FNwkSIntKey));
      memcpy1(lora_config.SNwkSIntKey, nwkskey, sizeof(lora_config.SNwkSIntKey));
  }

  // Application Section Key
  flag = readEEPROMByte(APP_SKEY_FLAG);
  if(flag == FLAG_CODE) {
      uint8_t appskey[16] = {0};
      uint32_t read_from_mem1 = readEEPROMByte(APP_SKEY_0);
      uint32_t read_from_mem2 = readEEPROMByte(APP_SKEY_1);
      uint32_t read_from_mem3 = readEEPROMByte(APP_SKEY_2);
      uint32_t read_from_mem4 = readEEPROMByte(APP_SKEY_3);

      // convert uint32_t to uint8_t
      appskey[0] = read_from_mem1 >> 24; appskey[1] = read_from_mem1 >> 16; appskey[2] = read_from_mem1 >> 8; appskey[3] = read_from_mem1;
      appskey[4] = read_from_mem2 >> 24; appskey[5] = read_from_mem2 >> 16; appskey[6] = read_from_mem2 >> 8; appskey[7] = read_from_mem2;
      appskey[8] = read_from_mem3 >> 24; appskey[9] = read_from_mem3 >> 16; appskey[10] = read_from_mem3 >> 8; appskey[11] = read_from_mem3;
      appskey[12] = read_from_mem4 >> 24; appskey[13] = read_from_mem4 >> 16; appskey[14] = read_from_mem4 >> 8; appskey[15] = read_from_mem4;

      // PPRINTF("appskey from memory = %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX16(appskey));
      memcpy1(lora_config.AppSKey, appskey, sizeof(lora_config.AppSKey));
  }

  // Tx Power
  flag = readEEPROMByte(TX_POWER_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.TxPower = readEEPROMByte(TX_POWER);
  }

  // Data rate
  flag = readEEPROMByte(DATARATE_FLAG);
  if(flag == FLAG_CODE) {
      lora_config_tx_datarate_set(readEEPROMByte(DATARATE));
  }

  // RX2 frequency
  flag = readEEPROMByte(RX2_FREQ_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.Rx2Frequency = readEEPROMByte(RX2_FREQ);
  }

  // RX2 datarate
  flag = readEEPROMByte(RX2_DATARATE_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.Rx2DataRate = readEEPROMByte(RX2_DATARATE);
  }

  // RX1 delay
  flag = readEEPROMByte(RX1_DELAY_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.Rx1Delay = readEEPROMByte(RX1_DELAY);
  }

  // RX2 delay
  flag = readEEPROMByte(RX2_DELAY_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.Rx2Delay = readEEPROMByte(RX2_DELAY);
  }

  // Join delay rx window 1
  flag = readEEPROMByte(JOIN_DELAY_1_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.JoinDelay1 = readEEPROMByte(JOIN_DELAY_1);
  }

  // Join delay rx window 2
  flag = readEEPROMByte(JOIN_DELAY_2_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.JoinDelay2 = readEEPROMByte(JOIN_DELAY_2);
  }

  // Authentication mode
  flag = readEEPROMByte(JOIN_MODE_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.otaa = readEEPROMByte(JOIN_MODE);
  }

  // Network id
  flag = readEEPROMByte(NWK_ID_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.netid = readEEPROMByte(NWK_ID);
  }

  // class
  flag = readEEPROMByte(CLASS_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.class = readEEPROMByte(CLASS);
  }

  // Confirmation mode
  flag = readEEPROMByte(CONF_MODE_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.ReqAck = readEEPROMByte(CONF_MODE);
  }

  // Frequency sub-band
  flag = readEEPROMByte(SUB_BAND_FLAG);
  if(flag == FLAG_CODE) {
      lora_config.subBand = readEEPROMByte(SUB_BAND);
  }
}

/**
 *  lora Init
 */
void LORA_Init (LoRaMainCallback_t *callbacks, LoRaParam_t* LoRaParam )
{
  /* init the Tx Duty Cycle*/
  LoRaParamInit = LoRaParam;

  /* init the main call backs*/
  LoRaMainCallbacks = callbacks;

#if (STATIC_DEVICE_EUI != 1)
  LoRaMainCallbacks->BoardGetUniqueId( lora_config.DevEui );
#endif

#if (STATIC_DEVICE_ADDRESS != 1)
  // Random seed initialization
  srand1( LoRaMainCallbacks->BoardGetRandomSeed( ) );
  // Choose a random device address
  DevAddr = randr( 0, 0x01FFFFFF );
#endif

  // inicializa parâmetros com valores salvos na memória
  get_params_from_memory();

  LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
  LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
  LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
  LoRaMacPrimitives.MacMlmeIndication = MlmeIndication;
  LoRaMacCallbacks.GetBatteryLevel = LoRaMainCallbacks->BoardGetBatteryLevel;
  LoRaMacCallbacks.GetTemperatureLevel = LoRaMainCallbacks->BoardGetTemperatureLevel;
#if defined( REGION_AS923 )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_AS923 );
#elif defined( REGION_AU915 )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_AU915 );
#elif defined( REGION_CN470 )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_CN470 );
#elif defined( REGION_CN779 )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_CN779 );
#elif defined( REGION_EU433 )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_EU433 );
 #elif defined( REGION_IN865 )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_IN865 );
#elif defined( REGION_EU868 )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_EU868 );
#elif defined( REGION_KR920 )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_KR920 );
#elif defined( REGION_US915 )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_US915 );
#elif defined( REGION_US915_HYBRID )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_US915_HYBRID );
#elif defined( REGION_RU864 )
  LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_RU864 );
#else
    #error "Please define a region in the compiler options."
#endif

  mibReq.Type = MIB_ADR;
  mibReq.Param.AdrEnable = LoRaParamInit->AdrEnable;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_PUBLIC_NETWORK;
  mibReq.Param.EnablePublicNetwork = LoRaParamInit->EnablePublicNetwork;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_APP_KEY;
  mibReq.Param.AppKey = lora_config.AppKey;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_NWK_KEY;
  mibReq.Param.NwkKey = lora_config.NwkKey;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_DEVICE_CLASS;
  mibReq.Param.Class = lora_config.class;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_CHANNELS_TX_POWER;
  mibReq.Param.ChannelsTxPower = lora_config.TxPower;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_RX2_CHANNEL;
  mibReq.Param.Rx2Channel.Frequency = lora_config.Rx2Frequency;
  LoRaMacMibSetRequestConfirm( &mibReq );
  mibReq.Param.Rx2Channel.Datarate = lora_config.Rx2DataRate;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_RECEIVE_DELAY_1;
  mibReq.Param.ReceiveDelay1 = lora_config.Rx1Delay;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_RECEIVE_DELAY_2;
  mibReq.Param.ReceiveDelay2 = lora_config.Rx2Delay;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_JOIN_ACCEPT_DELAY_1;
  mibReq.Param.JoinAcceptDelay1 = lora_config.JoinDelay1;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_JOIN_ACCEPT_DELAY_2;
  mibReq.Param.JoinAcceptDelay2 = lora_config.JoinDelay2;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_NET_ID;
  mibReq.Param.NetID = lora_config.netid;
  LoRaMacMibSetRequestConfirm( &mibReq );

#if defined( REGION_EU868 )
  LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON );

  lora_config.duty_cycle = LORA_DISABLE;
#else
  lora_config.duty_cycle = LORA_ENABLE;
#endif

  mibReq.Type = MIB_SYSTEM_MAX_RX_ERROR;
  mibReq.Param.SystemMaxRxError = 10;
  LoRaMacMibSetRequestConfirm( &mibReq );

  char channels = lora_config.subBand + '0';
  lora_set_sub_band(channels);

  // Set authentication mode
  lora_config_otaa_set(lora_config.otaa);

  /*set Mac statein Idle*/
  LoRaMacStart( );
}

LoraErrorStatus LORA_Join( void)
{
  LoraErrorStatus status;
  if (lora_config.otaa == LORA_ENABLE)
  {
    MlmeReq_t mlmeReq;

    mlmeReq.Type = MLME_JOIN;
    mlmeReq.Req.Join.DevEui = lora_config.DevEui;
    mlmeReq.Req.Join.JoinEui = lora_config.JoinEui;
    mlmeReq.Req.Join.Datarate = LoRaParamInit->TxDatarate;

    JoinParameters = mlmeReq.Req.Join;

    LoRaMacMlmeRequest( &mlmeReq );

    status = LORA_SUCCESS;
  }
  else
  { /*no Join in abp*/
    status = LORA_ERROR;
  }

  return status;
}

LoraFlagStatus LORA_JoinStatus( void)
{
  MibRequestConfirm_t mibReq;

  mibReq.Type = MIB_NETWORK_ACTIVATION;

  LoRaMacMibGetRequestConfirm( &mibReq );

  if( mibReq.Param.NetworkActivation == ACTIVATION_TYPE_NONE )
  {
    return LORA_RESET;
  }
  else
  {
    return LORA_SET;
  }
}

LoraErrorStatus LORA_send(lora_AppData_t* AppData, LoraConfirm_t IsTxConfirmed)
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;
    /*if certification test are on going, application data is not sent*/
    if (certif_running() == true)
    {
      return LORA_ERROR;
    }

    if( LoRaMacQueryTxPossible( AppData->BuffSize, &txInfo ) != LORAMAC_STATUS_OK )
    {
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = lora_config_tx_datarate_get() ;
    }
    else
    {
        if( IsTxConfirmed == LORAWAN_UNCONFIRMED_MSG )
        {
            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = AppData->Port;
            mcpsReq.Req.Unconfirmed.fBufferSize = AppData->BuffSize;
            mcpsReq.Req.Unconfirmed.fBuffer = AppData->Buff;
            mcpsReq.Req.Unconfirmed.Datarate = lora_config_tx_datarate_get() ;
        }
        else
        {
            mcpsReq.Type = MCPS_CONFIRMED;
            mcpsReq.Req.Confirmed.fPort = AppData->Port;
            mcpsReq.Req.Confirmed.fBufferSize = AppData->BuffSize;
            mcpsReq.Req.Confirmed.fBuffer = AppData->Buff;
            mcpsReq.Req.Confirmed.NbTrials = 8;
            mcpsReq.Req.Confirmed.Datarate = lora_config_tx_datarate_get() ;
        }
    }
    if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK )
    {
        return LORA_SUCCESS;
    }
    return LORA_ERROR;
}

#if defined( USE_DEVICE_TIMING )
static LoraErrorStatus LORA_DeviceTimeReq( void)
{
  MlmeReq_t mlmeReq;

  mlmeReq.Type = MLME_DEVICE_TIME;

  if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
  {
    return LORA_SUCCESS;
  }
  else
  {
    return LORA_ERROR;
  }
}
#else
static LoraErrorStatus LORA_BeaconTimeReq( void)
{
  MlmeReq_t mlmeReq;

  mlmeReq.Type = MLME_BEACON_TIMING;

  if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
  {
    return LORA_SUCCESS;
  }
  else
  {
    return LORA_ERROR;
  }
}
#endif

static LoraErrorStatus LORA_BeaconReq( void)
{
  MlmeReq_t mlmeReq;

  mlmeReq.Type = MLME_BEACON_ACQUISITION;

  if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
  {
    return LORA_SUCCESS;
  }
  else
  {
    return LORA_ERROR;
  }
}

static LoraErrorStatus LORA_PingSlotReq( void)
{

  MlmeReq_t mlmeReq;

  mlmeReq.Type = MLME_LINK_CHECK;
  LoRaMacMlmeRequest( &mlmeReq );

  mlmeReq.Type = MLME_PING_SLOT_INFO;
  mlmeReq.Req.PingSlotInfo.PingSlot.Fields.Periodicity = LORAWAN_DEFAULT_PING_SLOT_PERIODICITY;
  mlmeReq.Req.PingSlotInfo.PingSlot.Fields.RFU = 0;

  if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
  {
      return LORA_SUCCESS;
  }
  else
  {
     return LORA_ERROR;
  }
}

LoraErrorStatus LORA_RequestClass( DeviceClass_t newClass )
{
  LoraErrorStatus Errorstatus = LORA_SUCCESS;
  MibRequestConfirm_t mibReq;
  DeviceClass_t currentClass;

  mibReq.Type = MIB_DEVICE_CLASS;
  LoRaMacMibGetRequestConfirm( &mibReq );

  currentClass = mibReq.Param.Class;
  /*attempt to swicth only if class update*/
  if (currentClass != newClass)
  {
    switch (newClass)
    {
      case CLASS_A:
      {
        mibReq.Param.Class = CLASS_A;
        if( LoRaMacMibSetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK )
        {
        /*switch is instantanuous*/
          LoRaMainCallbacks->LORA_ConfirmClass(CLASS_A);
        }
        else
        {
          Errorstatus = LORA_ERROR;
        }
        break;
      }
      case CLASS_B:
      {
        mibReq.Param.Class = CLASS_B;
        if( LoRaMacMibSetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK )
        {
          /*switch is instantanuous*/
          LoRaMainCallbacks->LORA_ConfirmClass(CLASS_B);
        }
        else
        {
          Errorstatus = LORA_ERROR;
        }

        /*
#ifndef LORAMAC_CLASSB_ENABLED
        PRINTF( "warning: LORAMAC_CLASSB_ENABLED has not been defined at compilation\n\r");
#endif
        if (currentClass != CLASS_A)
        {
          Errorstatus = LORA_ERROR;
        }
        // switch is not instantanuous
        Errorstatus = LORA_BeaconReq( );
        */

        break;
      }
      case CLASS_C:
      {
        if (currentClass != CLASS_A)
        {
          Errorstatus = LORA_ERROR;
        }
        /*switch is instantanuous*/
        mibReq.Param.Class = CLASS_C;
        if( LoRaMacMibSetRequestConfirm( &mibReq ) == LORAMAC_STATUS_OK )
        {
          LoRaMainCallbacks->LORA_ConfirmClass(CLASS_C);
        }
        else
        {
            Errorstatus = LORA_ERROR;
        }
        break;
      }
      default:
        break;
    }
  }
  return Errorstatus;
}

void LORA_GetCurrentClass( DeviceClass_t *currentClass )
{
  MibRequestConfirm_t mibReq;

  mibReq.Type = MIB_DEVICE_CLASS;
  LoRaMacMibGetRequestConfirm( &mibReq );

  *currentClass = mibReq.Param.Class;
}

void lora_set_sub_band(char subBand) {
  MibRequestConfirm_t mib;
  uint16_t gatewayChannelsMask[5] = {0};

  switch (subBand) {
    case '0': {
            // enable all channels
            uint16_t mask[5] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x00FF };
            memcpy(gatewayChannelsMask, mask, sizeof(mask)+1);
            break;
    }
    case '1': {
            // 0001 0000 0000 0000 00FF - sub band 1
            uint16_t mask[5] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0001 };
            memcpy(gatewayChannelsMask, mask, sizeof(mask)+1);
            break;
    }
    case '2': {
            // 0002 0000 0000 0000 FF00 - sub band 2
            uint16_t mask[5] = { 0xFF00, 0x0000, 0x0000, 0x0000, 0x0002 };
            memcpy(gatewayChannelsMask, mask, sizeof(mask)+1);
            break;
    }
    case '3': {
            // 0004 0000 0000 00FF 0000 - sub band 3
            uint16_t mask[5] = { 0x0000, 0x00FF, 0x0000, 0x0000, 0x0004 };
            memcpy(gatewayChannelsMask, mask, sizeof(mask)+1);
            break;
    }
    case '4': {
            // 0008 0000 0000 FF00 0000 - sub band 4
            uint16_t mask[5] = { 0x0000, 0xFF00, 0x0000, 0x0000, 0x0008 };
            memcpy(gatewayChannelsMask, mask, sizeof(mask)+1);
            break;
    }
    case '5': {
            // 0100 0000 00FF 0000 0000 - sub band 5
            uint16_t mask[5] = { 0x0000, 0x0000, 0x00FF, 0x0000, 0x0100 };
            memcpy(gatewayChannelsMask, mask, sizeof(mask)+1);
            break;
    }
    case '6': {
            // 0200 0000 FF00 0000 0000 - sub band 6
            uint16_t mask[5] = { 0x0000, 0x0000, 0xFF00, 0x0000, 0x0200 };
            memcpy(gatewayChannelsMask, mask, sizeof(mask)+1);
            break;
    }
    case '7': {
            // 0400 00FF 0000 0000 0000 - sub band 7
            uint16_t mask[5] = { 0x0000, 0x0000, 0x0000, 0x00FF, 0x0400 };
            memcpy(gatewayChannelsMask, mask, sizeof(mask)+1);
            break;
    }
    case '8': {
            // 0800 FF00 0000 0000 0000 - sub band 8
            uint16_t mask[5] = { 0x0000, 0x0000, 0x0000, 0xFF00, 0x0800 };
            memcpy(gatewayChannelsMask, mask, sizeof(mask)+1);
            break;
    }
    default:
            break;
  }

  mib.Type = MIB_CHANNELS_DEFAULT_MASK;
  mib.Param.ChannelsDefaultMask = gatewayChannelsMask;
  LoRaMacMibSetRequestConfirm( &mib );

  mib.Type = MIB_CHANNELS_MASK;
  mib.Param.ChannelsMask = gatewayChannelsMask;
  LoRaMacMibSetRequestConfirm( &mib );
}

void lora_config_otaa_set(LoraState_t otaa)
{
  lora_config.otaa = otaa;

  if (lora_config.otaa == LORA_ENABLE)
  {
    PPRINTF("OTAA Mode enabled\n\r");
    PPRINTF("DevEui= %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX8(lora_config.DevEui));
    PPRINTF("JoinEui= %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX8(lora_config.JoinEui));
    PPRINTF("AppKey= %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\r", HEX16(lora_config.AppKey));

    mibReq.Type = MIB_NETWORK_ACTIVATION;
    mibReq.Param.NetworkActivation = ACTIVATION_TYPE_NONE;
    LoRaMacMibSetRequestConfirm( &mibReq );
  }
  else
  {
    PPRINTF("ABP Mode enabled\n\r");
    PPRINTF("DevEui= %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX8(lora_config.DevEui));
    PPRINTF("DevAdd=  %08X\n\r", DevAddr) ;
    PPRINTF("NwkSKey= %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\r", HEX16(lora_config.NwkSEncKey));
    PPRINTF("AppSKey= %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n\r", HEX16(lora_config.AppSKey));

    mibReq.Type = MIB_DEV_ADDR;
    mibReq.Param.DevAddr = DevAddr;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_F_NWK_S_INT_KEY;
    mibReq.Param.FNwkSIntKey = lora_config.FNwkSIntKey;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_S_NWK_S_INT_KEY;
    mibReq.Param.SNwkSIntKey = lora_config.SNwkSIntKey;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_NWK_S_ENC_KEY;
    mibReq.Param.NwkSEncKey = lora_config.NwkSEncKey;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_APP_S_KEY;
    mibReq.Param.AppSKey = lora_config.AppSKey;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_NETWORK_ACTIVATION;
    mibReq.Param.NetworkActivation = ACTIVATION_TYPE_ABP;
    LoRaMacMibSetRequestConfirm( &mibReq );

    // Enable legacy mode to operate according to LoRaWAN Spec. 1.0.3
    Version_t abpLrWanVersion;

    abpLrWanVersion.Fields.Major    = 1;
    abpLrWanVersion.Fields.Minor    = 0;
    abpLrWanVersion.Fields.Revision = 3;
    abpLrWanVersion.Fields.Rfu      = 0;

    mibReq.Type = MIB_ABP_LORAWAN_VERSION;
    mibReq.Param.AbpLrWanVersion = abpLrWanVersion;
    LoRaMacMibSetRequestConfirm( &mibReq );
  }
}


LoraState_t lora_config_otaa_get(void)
{
  return lora_config.otaa;
}

void lora_config_duty_cycle_set(LoraState_t duty_cycle)
{
  lora_config.duty_cycle = duty_cycle;
  LoRaMacTestSetDutyCycleOn((duty_cycle == LORA_ENABLE) ? 1 : 0);
}

LoraState_t lora_config_duty_cycle_get(void)
{
  return lora_config.duty_cycle;
}

uint8_t *lora_config_deveui_get(void)
{
  return lora_config.DevEui;
}

void lora_config_deveui_set(uint8_t deveui[8])
{
  memcpy1(lora_config.DevEui, deveui, sizeof(lora_config.DevEui));
  // PPRINTF("DevEui set= %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n\r", HEX8(lora_config.DevEui));
}

uint8_t *lora_config_joineui_get(void)
{
  return lora_config.JoinEui;
}

void lora_config_joineui_set(uint8_t joineui[8])
{
  memcpy1(lora_config.JoinEui, joineui, sizeof(lora_config.JoinEui));
}

uint8_t *lora_config_appkey_get(void)
{
  return lora_config.AppKey;
}

void lora_config_appkey_set(uint8_t appkey[16])
{
  memcpy1(lora_config.AppKey, appkey, sizeof(lora_config.AppKey));
}

void lora_config_reqack_set(LoraConfirm_t reqack)
{
  lora_config.ReqAck = reqack;
}

LoraConfirm_t lora_config_reqack_get(void)
{
  return lora_config.ReqAck;
}

int8_t lora_config_snr_get(void)
{
  return lora_config.Snr;
}

int16_t lora_config_rssi_get(void)
{
  return lora_config.Rssi;
}

void lora_config_tx_datarate_set(int8_t TxDataRate)
{
  lora_config.TxDatarate = TxDataRate;
}

int8_t lora_config_tx_datarate_get(void )
{
  return lora_config.TxDatarate;
}

LoraState_t lora_config_isack_get(void)
{
  if (lora_config.McpsConfirm == NULL)
  {
    return LORA_DISABLE;
  }
  else
  {
    return (lora_config.McpsConfirm->AckReceived ? LORA_ENABLE : LORA_DISABLE);
  }
}


/* Dummy data sent periodically to let the tester respond with start test command*/
static TimerEvent_t TxcertifTimer;

void OnCertifTimer( void)
{
  uint8_t Dummy[1]= {1};
  lora_AppData_t AppData;
  AppData.Buff=Dummy;
  AppData.BuffSize=sizeof(Dummy);
  AppData.Port = 224;

  LORA_send( &AppData, LORAWAN_UNCONFIRMED_MSG);
}

void lora_wan_certif( void )
{
  LoRaMacTestSetDutyCycleOn( false );
  LORA_Join( );
  TimerInit( &TxcertifTimer,  OnCertifTimer); /* 8s */
  TimerSetValue( &TxcertifTimer,  8000); /* 8s */
  TimerStart( &TxcertifTimer );

}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
