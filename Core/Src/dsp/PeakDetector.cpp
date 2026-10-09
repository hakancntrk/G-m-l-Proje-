/**
 * @file PeakDetector.cpp
 * @brief Top-N tepe tespiti ve parabolik interpolasyon.
 */

#include "dsp/PeakDetector.hpp"
#include <cmath>

// Parabolik interpolasyon ile tepe frekansını ve genliğini hassaslaştırır.
void PeakDetector::parabolicInterp(const float *magDb, uint32_t k, float &delta,
                                   float &interpMag) {
  const float left = magDb[k - 1];
  const float peak = magDb[k];
  const float right = magDb[k + 1];

  const float diff = left - right;
  const float denom = left - 2.0f * peak + right;

  // Düz tepe / sıfıra yakın payda: interpolasyon yapma
  if (fabsf(denom) < 1e-12f) {
    delta = 0.0f;
    interpMag = peak;
    return;
  }

  delta = 0.5f * diff / denom;
  interpMag = peak - 0.25f * diff * delta;
}

// Tepe tespiti ve Top-N seçimi
uint32_t PeakDetector::detect(const float *magDb) {
  peakCount_ = 0;

  // Başlangıçta tepe eşiklerini tabana çek
  for (uint32_t i = 0; i < cfg::MAX_PEAKS; ++i) {
    peaks_[i].magnitudeDb = -200.0f;
  }

  // 1) Tüm geçerli FFT bin'lerini tara
  for (uint32_t k = cfg::MIN_PEAK_BIN; k <= cfg::MAX_PEAK_BIN; ++k) {
    const float mag = magDb[k];

    // Eşik altı değerleri ve lokal tepe olmayanları atla
    if (mag < cfg::PEAK_THRESHOLD_DB)
      continue;

    if (mag <= magDb[k - 1] || mag <= magDb[k + 1])
      continue;

    // Liste doluysa ve yeni tepe listedeki en zayıftan küçükse atla
    if (peakCount_ == cfg::MAX_PEAKS &&
        mag <= peaks_[cfg::MAX_PEAKS - 1].magnitudeDb) {
      continue;
    }

    uint32_t pos;
    if (peakCount_ < cfg::MAX_PEAKS) {
      pos = peakCount_;
      ++peakCount_;
    } else {
      pos = cfg::MAX_PEAKS - 1;
    }

    // Küçük olanları bir sağa kaydır (Shift-right insertion)
    while (pos > 0 && peaks_[pos - 1].magnitudeDb < mag) {
      peaks_[pos] = peaks_[pos - 1];
      --pos;
    }

    // Doğru pozisyona doğrudan yerleştir (Gereksiz frekans hesabı yapmadan)
    peaks_[pos].bin = k;
    peaks_[pos].magnitudeDb = mag;
  }

  // 2) Sadece seçilen Top-N tepeye parabolik interpolasyon uygula
  for (uint32_t i = 0; i < peakCount_; ++i) {
    const uint32_t k = peaks_[i].bin;
    float delta, interpMag;

    parabolicInterp(magDb, k, delta, interpMag);

    peaks_[i].frequencyHz =
        (static_cast<float>(k) + delta) * cfg::FREQ_BIN_WIDTH_HZ;
    peaks_[i].magnitudeDb = interpMag;
  }

  return peakCount_;
}
