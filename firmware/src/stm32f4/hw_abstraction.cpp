#include <hw_abstraction.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

#include "stm32f4xx_hal.h"

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include <ws2812b/ws2812b.h>
#include <ssd1306.h>
#include <ssd1306_tests.h>

#define EEPROM_START_ADDRESS  ((uint32_t)0x08040000)
#define EEPROM_VALID  ((uint32_t)0xCACA)

#ifdef __cplusplus
}
#endif

#include <string>
#include <cstdarg>
#include <APDS9930.h>

static GPIO_InitTypeDef  GPIO_InitStructLed;
static GPIO_InitTypeDef  GPIO_InitStructButton;

static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);

TIM_HandleTypeDef htim3;
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;



uint8_t frameBuffer[3*4];

struct HarwareContext {
  HarwareContext(I2C_HandleTypeDef* pSensor1I2CH, I2C_HandleTypeDef* pSensor2I2CH)
  : mApds1(pSensor1I2CH)
  , mApds2(pSensor2I2CH){
    
  }
  
  APDS9930 mApds1;  
  APDS9930 mApds2;  
  uint16_t mEncoderCount;
  bool mEncoderPress;
};

HarwareContext& getHwContext(I2C_HandleTypeDef* pSensor1I2CH = nullptr, I2C_HandleTypeDef* pSensor2I2CH = nullptr){
  static HarwareContext mHwContext(pSensor1I2CH, pSensor2I2CH);
  return mHwContext;
}

bool hw_init(){
  /* -1- Enable GPIOE Clock (to be able to program the configuration registers) */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  
  /* -2- Configure PE.8 to PE.15 IOs in output push-pull mode to drive external LEDs */
  GPIO_InitStructLed.Pin = (GPIO_PIN_13);
  GPIO_InitStructLed.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructLed.Pull = GPIO_PULLUP;
  GPIO_InitStructLed.Speed = GPIO_SPEED_FREQ_HIGH;
  
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructLed);
  
  /* -2- Configure PE.8 to PE.15 IOs in output push-pull mode to drive external LEDs */
  GPIO_InitStructButton.Pin = (GPIO_PIN_0);
  GPIO_InitStructButton.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructButton.Pull = GPIO_PULLUP;
  
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructButton);
  
  
  MX_TIM3_Init();
  
  MX_USB_DEVICE_Init();
  
  MX_I2C1_Init();
  MX_I2C2_Init();
  
  HAL_Delay(500);
  
  // Set output channel/pin, GPIO_PIN_0 = 0, for GPIO_PIN_5 = 5 - this has to correspond to WS2812B_PINS
  ws2812b.item[0].channel = 8;
  // Your RGB framebuffer
  ws2812b.item[0].frameBufferPointer = frameBuffer;
  // RAW size of framebuffer
  ws2812b.item[0].frameBufferSize = sizeof(frameBuffer);

  ws2812b_init();
  
  getHwContext(&hi2c2, &hi2c1);
  
  getHwContext().mEncoderCount = TIM3->CNT;
  
  getHwContext().mEncoderPress = false;
  
  
  
  auto& lApds1 = getHwContext().mApds1;
  auto& lApds2 = getHwContext().mApds2;
  
  if (!(lApds1.init() && lApds2.init())){
    return false;
  } else {
    lApds1.enablePower();
    
    lApds2.enablePower();
    
  }
  
  ssd1306_Init();
  ssd1306_SetDisplayOn(1);
  
  if(HAL_OK == HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(0x39<<1), 2, 2)){
    print_dbg("APDS9930 1 ready\n");
  } else {
    print_dbg("APDS9930 1 NOT ready\n");
  }
  
  if(HAL_OK == HAL_I2C_IsDeviceReady(&hi2c1, SSD1306_I2C_ADDR, 2, 2)){
    print_dbg("OLED ready\n");
  } else {
    print_dbg("OLED NOT ready\n");
  }
  
  if(HAL_OK == HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(0x39<<1), 2, 2)){
    print_dbg("APDS9930 2 ready\n");
  } else {
    print_dbg("APDS9930 2 NOT ready\n");
  }
  
  
  #if 0
  while (1){
    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET){
      print_dbg("button pressed\n");
      
    } else {
      // set_start_lights(200,0,0);
    }
    auto ts1 = get_sensor_track_1();
    HAL_Delay(10);
    auto ts2 = get_sensor_track_2();
    if (ts1>500 || ts2>500){
      fprint_dbg("%3d \t%3d\n", ts1, ts2);
      
    }else{
    
      HAL_Delay(10);
    }
  }
  #endif
  
  
  return true;
}


void toggleDbgLed(){
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

void set_start_lights(uint8_t red, uint8_t green, uint8_t blue){
  if(ws2812b.transferComplete){
    // Signal that buffer is changed and transfer new data
    frameBuffer[0] = red;
    frameBuffer[1] = green;
    frameBuffer[2] = blue;
    frameBuffer[3] = red;
    frameBuffer[4] = green;
    frameBuffer[5] = blue;
    frameBuffer[6] = red;
    frameBuffer[7] = green;
    frameBuffer[8] = blue;
    ws2812b.startTransfer = 1;
    ws2812b_handle();
  }
}

void set_start_lights3(uint8_t red1, uint8_t green1, uint8_t blue1, uint8_t red2, uint8_t green2, uint8_t blue2, uint8_t red3, uint8_t green3, uint8_t blue3){
  if(ws2812b.transferComplete){
    // Signal that buffer is changed and transfer new data
    frameBuffer[0] = red1;
    frameBuffer[1] = green1;
    frameBuffer[2] = blue1;
    frameBuffer[3] = red2;
    frameBuffer[4] = green2;
    frameBuffer[5] = blue2;
    frameBuffer[6] = red3;
    frameBuffer[7] = green3;
    frameBuffer[8] = blue3;
    ws2812b.startTransfer = 1;
    ws2812b_handle();
  }
}

EncoderStatus get_encoder_status(){
  EncoderStatus lStatus = Stall;
  int lCount = TIM3->CNT >> 2;
  if (lCount != getHwContext().mEncoderCount){
    if (lCount > getHwContext().mEncoderCount){
      lStatus = Increment;
    }else{
      lStatus = Decrement;
    }
    getHwContext().mEncoderCount = lCount;
  }
  return lStatus;
}

bool get_encoder_pressed(){
  bool lEncBtn = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET);
  bool lRes = (lEncBtn!=getHwContext().mEncoderPress) ? lEncBtn : false;
  getHwContext().mEncoderPress = lEncBtn;
  return lRes;
}

uint16_t get_sensor_track_1(){
  getHwContext().mApds1.enableProximitySensor(false);
  while(!(getHwContext().mApds1.getStatus()&0x2)){
    fprint_dbg("no prox:%d\n", getHwContext().mApds1.getStatus());
  }
  uint16_t lProx = 0;
  if (getHwContext().mApds1.readProximity(lProx) == true){
    return lProx;
  }
  
  getHwContext().mApds1.disableProximitySensor();
  return -1;
}

uint16_t get_sensor_track_2(){
  getHwContext().mApds2.enableProximitySensor(false);
  while(!(getHwContext().mApds2.getStatus()&0x2)){
    print_dbg("no prox\n");
  }
  uint16_t lProx = 0;
  if (getHwContext().mApds2.readProximity(lProx) == true){
    return lProx;
  }
  
  getHwContext().mApds2.disableProximitySensor();
  return -1;
}

void sleep_ms(int ms){
  HAL_Delay(ms);
}

void print_dbg(const char* msg){
  CDC_Transmit_FS(reinterpret_cast<unsigned char*>(const_cast<char*>(msg)), strlen(msg));
}

void fprint_dbg(const char* format, ...) {
  std::string lRes(strlen(format) * 2 + 30, '\0');
  va_list arglist;
  va_start(arglist, format);
  auto len = vsprintf(&lRes[0], format, arglist);
  va_end(arglist);
  lRes.resize(len);
  print_dbg(&lRes[0]);
}

uint32_t get_clock_ms() {
  return HAL_GetTick();
}

  
bool eeprom_write(uint32_t pOffset, uint8_t* pBuffer, uint32_t pSize){
  fprint_dbg("write %u (%u)at %p\n", (pSize==1)?(uint32_t)*pBuffer : (pSize==2)?(uint32_t)(*(uint16_t*)pBuffer):(pSize==4)?(uint32_t)(*(uint32_t*)pBuffer):(uint32_t)*pBuffer, pSize, pOffset);
  if (*reinterpret_cast<uint16_t*>(EEPROM_START_ADDRESS) != EEPROM_VALID){
    eeprom_clear();
    fprint_dbg("eeprom clean, setting up %u\n", *reinterpret_cast<uint16_t*>(EEPROM_START_ADDRESS));
    HAL_FLASH_Unlock();
    auto res = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, EEPROM_START_ADDRESS, static_cast<uint16_t>(EEPROM_VALID));
    if (HAL_OK != res){
      fprint_dbg("error HAL_FLASH_Program %u : %u\n", __LINE__, res);
      // HAL_FLASH_Lock();
      return false;
    } else {
      fprint_dbg("ok HAL_FLASH_Program %u : %u\n", __LINE__, HAL_FLASH_GetError());
    }
  } else {
    HAL_FLASH_Unlock();
  }
  
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGSERR | FLASH_FLAG_PGAERR);
  
  pOffset += 2;
  uint32_t lPtrOffset = 0;
  for ( ; lPtrOffset<pSize ; ){
    /*if (pSize - lPtrOffset >= 4){
      auto res = HAL_FLASH_Program(TYPEPROGRAM_WORD, EEPROM_START_ADDRESS+pOffset+lPtrOffset, *reinterpret_cast<uint32_t*>(pBuffer+lPtrOffset));
      fprint_dbg("write %u (%u)at %p\n", *reinterpret_cast<uint32_t*>(pBuffer+lPtrOffset), pSize, EEPROM_START_ADDRESS+pOffset+lPtrOffset);
      if (HAL_OK != res){
        fprint_dbg("error HAL_FLASH_Program %u : %u,%u\n", __LINE__, res, HAL_FLASH_GetError());
      }else {
        fprint_dbg("ok HAL_FLASH_Program %u : %u\n", __LINE__, HAL_FLASH_GetError());
      }
      lPtrOffset+= 4;
    } else */if (pSize - lPtrOffset >= 2){
      fprint_dbg("write %u (%u)at %p (%u)\n", *reinterpret_cast<uint16_t*>(pBuffer+lPtrOffset), pSize, EEPROM_START_ADDRESS+pOffset+lPtrOffset, *(uint16_t*)(EEPROM_START_ADDRESS+pOffset+lPtrOffset));
      auto res = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, EEPROM_START_ADDRESS+pOffset+lPtrOffset, *reinterpret_cast<uint16_t*>(pBuffer+lPtrOffset));
      FLASH_WaitForLastOperation(200);
      if (HAL_OK != res){
        fprint_dbg("error HAL_FLASH_Program %u : %u,%u\n", __LINE__, res, HAL_FLASH_GetError());
      }else {
        fprint_dbg("ok HAL_FLASH_Program %u : %u\n", __LINE__, *(uint16_t*)(EEPROM_START_ADDRESS+pOffset+lPtrOffset));
      }
      lPtrOffset+= 2;
    } else {
      fprint_dbg("write %u (%u)at %p\n", *pBuffer, pSize, EEPROM_START_ADDRESS+pOffset+lPtrOffset);
      auto res = HAL_FLASH_Program(TYPEPROGRAM_BYTE, EEPROM_START_ADDRESS+pOffset+lPtrOffset, *reinterpret_cast<uint8_t*>(pBuffer+lPtrOffset));
      if (HAL_OK != res){
        fprint_dbg("error HAL_FLASH_Program %u : %u\n", __LINE__, res);
      }else {
        fprint_dbg("ok HAL_FLASH_Program %u : %u\n", __LINE__, *(uint8_t*)(EEPROM_START_ADDRESS+pOffset+lPtrOffset));
      }
      lPtrOffset++;
    }
    
  }
  HAL_FLASH_Lock();
  return true;
}

bool eeprom_read(uint32_t pOffset, uint32_t pSize, uint8_t* pBuffer){
  if (*reinterpret_cast<uint16_t*>(EEPROM_START_ADDRESS) != EEPROM_VALID){
    fprint_dbg("eeprom not setup %u\n", *reinterpret_cast<uint16_t*>(EEPROM_START_ADDRESS));
    return false;
  }
  pOffset += 2;
  fprint_dbg("reading size %u from %p\n", pSize, EEPROM_START_ADDRESS+pOffset);
  memcpy(pBuffer, reinterpret_cast<void*>(EEPROM_START_ADDRESS+pOffset), pSize);
  fprint_dbg("read %u (%u) at %p\n", (pSize==1)?(uint32_t)*pBuffer : (pSize==2)?(uint32_t)(*(uint16_t*)pBuffer):(pSize==4)?(uint32_t)(*(uint32_t*)pBuffer):(uint32_t)*pBuffer, (pSize==1)?(uint32_t)(*(uint8_t*)(EEPROM_START_ADDRESS+pOffset)) : (pSize==2)?(uint32_t)(*(uint16_t*)(EEPROM_START_ADDRESS+pOffset)):(pSize==4)?(uint32_t)(*(uint32_t*)(EEPROM_START_ADDRESS+pOffset)):*(uint8_t*)(EEPROM_START_ADDRESS+pOffset), EEPROM_START_ADDRESS+pOffset);
  {
    fprint_dbg("ok flash read %u : %u\n", __LINE__, HAL_FLASH_GetError());
  }
  return true;
}

bool eeprom_clear(){
  print_dbg("clearing eeprom\n");
  HAL_StatusTypeDef FlashStatus = FLASH_WaitForLastOperation(200);
  if (FlashStatus != HAL_OK){
    fprint_dbg("error HAL_FLASHEx_Erase %u : %u, %u\n", __LINE__, FlashStatus, HAL_FLASH_GetError());
  }
  print_dbg("unlock eeprom\n");
  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGSERR | FLASH_FLAG_PGAERR);
  FLASH_EraseInitTypeDef pEraseInit;
  uint32_t SectorError = 0;
  pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;  
  pEraseInit.Sector = FLASH_SECTOR_6;
  pEraseInit.NbSectors = 1;
  pEraseInit.VoltageRange = (uint8_t)FLASH_VOLTAGE_RANGE_4;
  FlashStatus = HAL_FLASHEx_Erase(&pEraseInit, &SectorError); 
  /* If erase operation was failed, a Flash error code is returned */
  HAL_FLASH_Lock();
  if (FlashStatus != HAL_OK)
  {
    fprint_dbg("error HAL_FLASHEx_Erase %u : %u\n", __LINE__, FlashStatus);
    return false;
  }
  
}

void screenClear(){
  ssd1306_Fill(Black);
}

void screenUpdate(){
  ssd1306_UpdateScreen();
}

void screenWrite(const char* text, FontSize size, uint8_t x, uint8_t y){
  FontDef font =  size==Font_8 ? Font_6x8 :
                  size==Font_10? Font_7x10:
                  size==Font_18? Font_11x18:
                  size==Font_26? Font_16x26:
                  Font_6x8;
  ssd1306_SetCursor(x, y);
  ssd1306_WriteString(const_cast<char*>(text), font, White);
}

void screenLineDraw(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2){
  ssd1306_Line(x1, y1, x2, y2, White);
}

void screenClean(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2){
  ssd1306_DrawRectangle(x1, y1, x2, y2, Black);
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0x0F;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0x0F;
  if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  
  HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_1|TIM_CHANNEL_2);
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

static void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  HAL_I2C_Init(&hi2c1);

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}


