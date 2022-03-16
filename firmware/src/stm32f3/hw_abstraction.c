#include <hw_abstraction.h>


#include "stm32f3xx.h"

#include "stm32f3xx_hal.h"

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_interface.h"

#include <ws2812b/ws2812b.h>

uint16_t gEncoderCount;

static GPIO_InitTypeDef  GPIO_InitStructLed;

USBD_HandleTypeDef hUSBDDevice;


TIM_Encoder_InitTypeDef encoder;
TIM_HandleTypeDef timer;

uint8_t frameBuffer[3*1];

void hw_init(){
  /* -1- Enable GPIOE Clock (to be able to program the configuration registers) */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* -2- Configure PE.8 to PE.15 IOs in output push-pull mode to drive external LEDs */
  GPIO_InitStructLed.Pin = (GPIO_PIN_13);
  GPIO_InitStructLed.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructLed.Pull = GPIO_PULLUP;
  GPIO_InitStructLed.Speed = GPIO_SPEED_FREQ_HIGH;
  
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructLed);
  
  // Set output channel/pin, GPIO_PIN_0 = 0, for GPIO_PIN_5 = 5 - this has to correspond to WS2812B_PINS
  ws2812b.item[0].channel = 0;
  // Your RGB framebuffer
  ws2812b.item[0].frameBufferPointer = frameBuffer;
  // RAW size of framebuffer
  ws2812b.item[0].frameBufferSize = sizeof(frameBuffer);


  ws2812b_init();
  
  frameBuffer[3] = 0xFF;
  frameBuffer[4] = 0x00;
  frameBuffer[5] = 0x00;
  
  timer.Instance = TIM3;
  timer.Init.Period = 0xFFFF;
  timer.Init.CounterMode = TIM_COUNTERMODE_UP;
  timer.Init.Prescaler = 0;
  timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  
  encoder.EncoderMode = TIM_ENCODERMODE_TI12;
  
  encoder.IC1Filter = 0x0F;
  encoder.IC1Polarity = TIM_INPUTCHANNELPOLARITY_RISING;
  encoder.IC1Prescaler = TIM_ICPSC_DIV1;
  encoder.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  
  encoder.IC2Filter = 0x0F;
  encoder.IC2Polarity = TIM_INPUTCHANNELPOLARITY_RISING;
  encoder.IC2Prescaler = TIM_ICPSC_DIV1;
  encoder.IC2Selection = TIM_ICSELECTION_DIRECTTI;
 
  HAL_TIM_Encoder_Init(&timer, &encoder);
  
  HAL_TIM_Encoder_Start(&timer,TIM_CHANNEL_1);
  
  gEncoderCount = TIM3->CNT;
  
  /* Init Device Library */
  USBD_Init(&hUSBDDevice, &VCP_Desc, 0);

  /* Add Supported Class */
  USBD_RegisterClass(&hUSBDDevice, &USBD_CDC);

  /* Add CDC Interface Class */
  USBD_CDC_RegisterInterface(&hUSBDDevice, &USBD_CDC_fops);

  /* Start Device Process */
  USBD_Start(&hUSBDDevice);
}


void toggleDbgLed(){
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

void set_start_lights(uint16_t red, uint16_t green, uint16_t blue){
  if(ws2812b.transferComplete){
    // Signal that buffer is changed and transfer new data
    frameBuffer[0] = red;
    frameBuffer[1] = green;
    frameBuffer[2] = blue;
    ws2812b.startTransfer = 1;
    ws2812b_handle();
  }
}

EncoderStatus get_encoder_status(){
  EncoderStatus lStatus = Stall;
  auto lCount = TIM3->CNT >> 2;
  if (lCount != gEncoderCount){
    if (lCount > gEncoderCount){
      lStatus = Increment;
    }else{
      lStatus = Decrement;
    }
    gEncoderCount = lCount;
  }
  return lStatus;
}

bool get_encoder_pressed(){
  return 0;
}

void sleep_ms(int ms){
  HAL_Delay(ms);
}

void print_dbg(char* msg){
  CDC_Output_Data(msg, strlen(msg));
}

uint32_t get_clock_ms() {
  return HAL_GetTick();
}

void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef *htim) {
  GPIO_InitTypeDef GPIO_InitStruct;

  if (htim->Instance == TIM3) {

    __HAL_RCC_TIM3_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 1);

    HAL_NVIC_EnableIRQ(TIM3_IRQn);
  }
}


