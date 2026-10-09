/**
 * @file    configurer.cpp
 * @brief   Yapıştırıcı katman — nesneleri oluşturur, bağlar ve çalıştırır.
 *
 * Bu dosya tüm katmanları bir araya getirir:
 *   1. HAL handle'larını (CubeMX'in oluşturduğu) alır
 *   2. Kendi sınıflarımızdan nesneler oluşturur
 *   3. HAL callback'lerini samp
 ler'a yönlendirir
 *   4. Sonsuz döngüyü (super-loop) çalıştırır
 *
 * ═══════════════════════════════════════════════════════════════
 * PIN ÖZETİ (CubeMX'te ayarla):
 *
 *   PA0  → ADC1_IN1       (MAX4466 analog çıkışı)
 *   PA2  → USART2_TX      (Nucleo ST-Link VCP)
 *   PA3  → USART2_RX      (Nucleo ST-Link VCP)
 *   PB13 → SPI2_SCK       (ST7735 saat)
 *   PB15 → SPI2_MOSI      (ST7735 veri)
 *   PB0  → GPIO_Output    (ST7735 DC)
 *   PB1  → GPIO_Output    (ST7735 RST)
 *   PB2  → GPIO_Output    (ST7735 CS)
 *   PA5  → LD2            (Green Led)
 *
 *   TIM6 → TRGO = Update Event
 *          ARR  = (APB1_CLK / 48000) - 1
 *
 *   DMA1_Channel1 → ADC1, Circular, Half-word, HT+TC interrupt
 * ═══════════════════════════════════════════════════════════════
 */
#include "stm32f3xx_hal.h"

// Kendi sınıflarımız
#include "app/MainApp.hpp"
#include "hal/STM32AdcDmaSampler.hpp"
#include "hal/St7735Display.hpp"
#include "hal/UartLogSink.hpp"

// Handle'lar artık extern olarak çekilmiyor,
// main.c'den configurer fonksiyonuna parametre olarak geliyor.

// ════════════════════════════════════════════════════════════════
// KESMELER İÇİN GLOBAL POINTER
// ════════════════════════════════════════════════════════════════

// Sampler pointer'ı — HAL callback'lerinden erişmemiz gerekiyor
static STM32AdcDmaSampler *s_sampler = nullptr;


// ════════════════════════════════════════════════════════════════
// HAL DMA CALLBACK'LERİ
// ════════════════════════════════════════════════════════════════

/// DMA, tamponun İLK YARISINI doldurduğunda çağrılır.
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
  // Doğru ADC'den geldiğinden emin ol (birden fazla ADC olabilir)
  if (hadc->Instance == ADC1 && s_sampler) {
    s_sampler->onHalfTransferComplete();
  }
}
/// DMA, tamponun TAMAMINI doldurduğunda çağrılır.
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1 && s_sampler) {
    s_sampler->onTransferComplete();
  }
}

// ════════════════════════════════════════════════════════════════
// ANA GİRİŞ NOKTASI
// ════════════════════════════════════════════════════════════════

extern "C" void configurer(ADC_HandleTypeDef *hadc, TIM_HandleTypeDef *htim, SPI_HandleTypeDef *hspi, UART_HandleTypeDef *huart) {
  // ── 1. Nesneleri statik olarak oluştur (Çok daha basit!) ──
  // Handle'lar parametre olarak geldiği için isim değişse bile burada hata almayız!
  static STM32AdcDmaSampler sampler(hadc, htim);
  static St7735Display display(hspi);
  static UartLogSink logSink(huart);
  static MainApp app(sampler, display, logSink);

  // HAL callback'leri için sampler pointer'ını sakla
  s_sampler = &sampler;

  // ── 2. Ekranı başlat ──
  display.init(); // Reset → init komutları → temizle

  // ── 3. ADC kalibrasyonu ──
  HAL_ADCEx_Calibration_Start(hadc, ADC_SINGLE_ENDED);

  // ── 4. Uygulamayı başlat ──
  app.init(); // Splash screen → sampler başlat → state machine başlat

  // ── 5. Sonsuz döngü (super-loop) ──

  while (true) {
    app.tick(); // Durum makinesini bir adım ilerlet
  }
}
