/**
 * @file    STM32AdcDmaSampler.cpp
 * @brief   Ping-Pong ADC+DMA örnekleyici implementasyonu.
 */

#include "hal/STM32AdcDmaSampler.hpp"

STM32AdcDmaSampler::STM32AdcDmaSampler(ADC_HandleTypeDef *hadc,
                                       TIM_HandleTypeDef *htim)
    : hadc_(hadc), htim_(htim), dmaBuffer_{}, readyHalf_(0), produced_(0),
      consumed_(0) {}

bool STM32AdcDmaSampler::start() {
  // Temiz bir başlangıç için durum değişkenlerini sıfırla
  readyHalf_ = 0;
  produced_ = 0;
  consumed_ = 0;

  // Ping-pong tamponlama (Circular mode) ile DMA üzerinden ADC'yi başlat
  if (HAL_ADC_Start_DMA(hadc_, reinterpret_cast<uint32_t *>(dmaBuffer_),
                        2u * cfg::FFT_SIZE) != HAL_OK) {
    return false;
  }

  // ADC dönüşümlerini tetiklemek için zamanlayıcıyı (timer) başlat
  if (HAL_TIM_Base_Start(htim_) != HAL_OK) {
    return false;
  }

  return true;
}

bool STM32AdcDmaSampler::frameReady() const { return readyHalf_ != 0; }

const uint16_t *STM32AdcDmaSampler::frame() const {
  if (readyHalf_ == 2) {
    return &dmaBuffer_[cfg::FFT_SIZE];
  } else {
    return &dmaBuffer_[0];
  }
}

void STM32AdcDmaSampler::release() {
  readyHalf_ = 0;
  consumed_ = consumed_ + 1u;
}

/**
 * @brief Atlanan (işlenemeyen) çerçeve sayısını hesaplar.
 *
 * Eğer çizim/işleme süresi yarım-tampon dolum süresini (örn. ≈42.7 ms) aşarsa,
 * DMA henüz işlenmemiş verinin üzerine yazabilir. Bu fonksiyon, üretilen ve
 * tüketilen çerçeveler arasındaki farkı hesaplayarak kayıpları (frame drop)
 * tespit eder.
 */
uint32_t STM32AdcDmaSampler::droppedFrames() const {
  const uint32_t p = produced_;
  const uint32_t c = consumed_;
  uint32_t pending;

  if (readyHalf_ != 0) {
    pending = 1u;
  } else {
    pending = 0u;
  }

  return p - c - pending;
}

// -----------------------------------------------------------------------------
// Kesme Servis Rutinleri (ISR - Interrupt Service Routines)
// -----------------------------------------------------------------------------

void STM32AdcDmaSampler::onHalfTransferComplete() {
  readyHalf_ = 1;
  produced_ = produced_ + 1u;
}

void STM32AdcDmaSampler::onTransferComplete() {
  readyHalf_ = 2;
  produced_ = produced_ + 1u;
}
