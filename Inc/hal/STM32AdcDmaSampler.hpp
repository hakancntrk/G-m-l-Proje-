/**
 * @file    STM32AdcDmaSampler.hpp
 * @brief   ISampleSource arayüzünü gerçekleştiren sınıf.
 *          TIM6 tetiklemeli ADC1 + DMA ile sürekli ses örnekleme yapar.
 *
 * Donanım bağlantısı:
 *   PA0  (ADC1_IN1) ← MAX4466 analog çıkışı
 *   TIM6 → TRGO → ADC1'i tetikler (her 1/48000 saniyede bir)
 *   DMA1 Channel1 → ADC1 DR → dmaBuffer_ dizisine aktarır
 */
#ifndef STM32_ADC_DMA_SAMPLER_HPP
#define STM32_ADC_DMA_SAMPLER_HPP

#include "Configurer.hpp"               // cfg::FFT_SIZE vb.
#include "interfaces/ISampleSource.hpp" // Üst sınıf (arayüz)
#include "stm32f3xx_hal.h"

class STM32AdcDmaSampler : public ISampleSource {
public:
  STM32AdcDmaSampler(ADC_HandleTypeDef *hadc, TIM_HandleTypeDef *htim);

  bool start() override;                  // ADC+DMA'yı başlat
  bool frameReady() const override;       // Veri hazır mı?
  const uint16_t *frame() const override; // Hazır veriyi göster

  void release() override; // "Aldım" de

  uint32_t droppedFrames() const override; // Atlanan çerçeve sayısı

  // ── Kesme callback'leri (ISR'dan çağrılır) ──
  void onHalfTransferComplete(); // DMA ilk yarıyı doldurdu
  void onTransferComplete();     // DMA ikinci yarıyı doldurdu

private:
  ADC_HandleTypeDef *hadc_; // ADC handle pointer'ı
  TIM_HandleTypeDef *htim_; // Timer handle pointer'ı

  // ── Ping-pong tampon ──
  // Boyut: 2 × 2048 = 4096 uint16_t = 8192 byte = 8 KB
  uint16_t dmaBuffer_[2u * cfg::FFT_SIZE];

  // DMA kesmesi bu değişkeni günceller:
  volatile uint8_t readyHalf_; // 0=yok, 1=ilk yarı, 2=ikinci yarı

  // Çerçeve muhasebesi: üretilen (ISR) ve tüketilen (ana döngü) çerçeve
  // sayıları. atlanan = üretilen - tüketilen - (bekleyen varsa 1)
  volatile uint32_t produced_;
  volatile uint32_t consumed_;
};

#endif // STM32_ADC_DMA_SAMPLER_HPP
