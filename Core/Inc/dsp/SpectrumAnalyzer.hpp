/**
 * @file    SpectrumAnalyzer.hpp
 * @brief   Ses sinyalini frekans bileşenlerine ayıran DSP modülü.
 */
#ifndef SPECTRUM_ANALYZER_HPP
#define SPECTRUM_ANALYZER_HPP

#define __FPU_PRESENT 1U
#include "Configurer.hpp"
#include "arm_math.h" // ARM_MATH_CM4 derleyici ayarlarından global gelir
#include "main.h" // __FPU_PRESENT ve STM32 donanım tanımlarını otomatik getirir
#include <cstdint>

class SpectrumAnalyzer {
public:
  /// Yapıcı: Hann pencere katsayılarını hesaplar, FFT'yi başlatır.
  SpectrumAnalyzer();

  void process(const uint16_t *adcSamples);

  /// dB cinsinden genlik spektrumu (cfg::HALF_FFT eleman).
  /// dB = 20 × log10(lineer_genlik)
  const float *magnitudeDb() const { return magDb_; }

  /// Lineer genlik spektrumu (cfg::HALF_FFT eleman).
  /// Pitch tahmini için bu gerekli (dB ile aritmetik işlem yapılmaz).
  const float *magnitudeLin() const { return magLin_; }

private:
  // ── Çalışma tamponları (working buffers) ──

  float samples_[cfg::FFT_SIZE]; // Float'a çevrilmiş ADC verileri
  float fftBuf_[cfg::FFT_SIZE];  // FFT'nin çalışma alanı
  float magLin_[cfg::HALF_FFT];  // Lineer genlik sonuçları
  float magDb_[cfg::HALF_FFT];   // dB genlik sonuçları

  // Hann pencere katsayıları (constructor'da bir kez hesaplanır)
  float hannWindow_[cfg::FFT_SIZE];

  // CMSIS-DSP FFT yapısı.
  // arm_rfft_fast_init_f32() ile başlatılır, arm_rfft_fast_f32() ile
  // kullanılır.
  arm_rfft_fast_instance_f32 fftInstance_;
};

#endif // SPECTRUM_ANALYZER_HPP
