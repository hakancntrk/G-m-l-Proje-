/**
 * @file    SpectrumAnalyzer.cpp
 * @brief   DSP zinciri implementasyonu.
 *
 * Her adım ayrı bir fonksiyon olarak yazıldı.
 * process() hepsini sırayla çağırır.
 */

#include "dsp/SpectrumAnalyzer.hpp"
#include <cmath>
#include <cstring>

SpectrumAnalyzer::SpectrumAnalyzer() {
  // Hann penceresini bir kere hesapla (periyodik DFT standardı)
  for (uint32_t n = 0; n < cfg::FFT_SIZE; ++n) {
    hannWindow_[n] =
        0.5f * (1.0f - cosf(2.0f * 3.14159265f * n / cfg::FFT_SIZE));
  }
  arm_rfft_fast_init_f32(&fftInstance_, cfg::FFT_SIZE);

  // Başlangıçta tamponları sıfırla
  memset(magLin_, 0, sizeof(magLin_));
  memset(magDb_, 0, sizeof(magDb_));
}

void SpectrumAnalyzer::process(const uint16_t *adc) {
  const uint32_t N = cfg::FFT_SIZE;

  // 1) Ortalamayı (DC) bul (Hızlı int toplama, overflow riski yok)
  uint32_t sum = 0;
  for (uint32_t i = 0; i < N; ++i) {
    sum += adc[i];
  }
  const float mean = static_cast<float>(sum) / static_cast<float>(N);

  // 2) DC'yi çıkar, -1..+1 aralığına normalize et, Hann penceresini uygula
  const float half = cfg::ADC_MAX_VALUE * 0.5f;
  const float invHalf = 1.0f / half; // FPU bölme yerine çarpma için

  for (uint32_t i = 0; i < N; ++i) {
    samples_[i] =
        (static_cast<float>(adc[i]) - mean) * invHalf * hannWindow_[i];
  }

  // 3) Gerçel FFT (RFFT)
  arm_rfft_fast_f32(&fftInstance_, samples_, fftBuf_, 0);

  // 4) Genlik hesabı (DC için 2/N, AC harmonikler için Hann kaybı telafisiyle
  // 4/N)
  const float linScale = 4.0f / static_cast<float>(N);
  magLin_[0] = fabsf(fftBuf_[0]) * 2.0f / static_cast<float>(N);

  for (uint32_t k = 1; k < cfg::HALF_FFT; ++k) {
    float re = fftBuf_[2 * k];
    float im = fftBuf_[2 * k + 1];
    magLin_[k] = sqrtf(re * re + im * im) * linScale;
  }

  // 5) dB dönüşümü (1e-6 taban = -120 dB sınırı)
  for (uint32_t k = 0; k < cfg::HALF_FFT; ++k) {
    magDb_[k] = 20.0f * log10f(fmaxf(magLin_[k], 1e-6f));
  }
}
