/**
 * @file    PitchEstimator.hpp
 * @brief   Temel frekans (f0) tahmini — harmonik skorlama yöntemiyle.
 *
 */
#ifndef PITCH_ESTIMATOR_HPP
#define PITCH_ESTIMATOR_HPP

#include "Configurer.hpp"
#include "dsp/PeakDetector.hpp" // SpectralPeak struct'ını kullanacağız
#include <cstddef>
#include <cstdint>

/// Pitch tahmini sonucu.
struct PitchResult {
  float f0Hz; // Tahmini temel frekans (Hz), 0 ise bulunamadı

  char noteName[8];    // Nota adı: "A4", "C#5", "Eb3" gibi
  float centDeviation; // En yakın notadan sapma (+tiz, -pes)
  bool valid;          // true = güvenilir bir pitch bulundu
};

class PitchEstimator {
public:
  PitchEstimator() {
    // 0.0f geçerli bir MIDI değeri (C-1) olduğu için "pitch yok" işaretiyle başla
    for (uint32_t i = 0; i < cfg::STABILITY_FRAMES; ++i)
      history_[i] = cfg::NO_PITCH_MIDI;
  }

  /**
   * Temel frekansı tahmin et.
   *
   * @param peaks       PeakDetector'ın bulduğu tepeler (güçlüden zayıfa
   * sıralı).
   * @param peakCount   Kaç tepe var.
   * @param magLin      Lineer genlik spektrumu (interpolasyon için).
   * @return            Tahmin sonucu (f0, nota, cent, güvenilirlik).
   */
  PitchResult estimate(const SpectralPeak *peaks, uint32_t peakCount,
                       const float *magLin);

private:
  /// Rastgele bir frekanstaki genliği lineer interpolasyonla bul.
  static float magAtFreq(const float *magLin, float freqHz);

  /// Frekansı nota adına ve cent sapmasına dönüştür.
  static void freqToNote(float freqHz, char *nameOut, size_t nameSize,
                         float &centOut);

  float history_[cfg::STABILITY_FRAMES]; // Son karelerin MIDI değerleri (yoksa NO_PITCH_MIDI)
  uint32_t histIdx_ = 0;
};

#endif // PITCH_ESTIMATOR_HPP
