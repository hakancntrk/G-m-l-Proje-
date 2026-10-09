/**
 * @file    PeakDetector.hpp
 * @brief   Frekans spektrumunda en güçlü N tepe noktasını bulan modül.
 */
#ifndef PEAK_DETECTOR_HPP
#define PEAK_DETECTOR_HPP

#include "Configurer.hpp"
#include <cstdint>
struct SpectralPeak {
  float frequencyHz; // İnterpolasyon sonrası frekans (Hz)
  float magnitudeDb; // İnterpolasyon sonrası genlik (dB)
  uint32_t bin;      // Tam sayı bin indeksi (FFT çıkışındaki konum)
};

class PeakDetector {
public:
  PeakDetector() = default;

  /**
   * Genlik spektrumunda tepe noktalarını tespit et.
   *
   * @param magDb  dB cinsinden genlik spektrumu (cfg::HALF_FFT eleman).
   * @return       Bulunan tepe sayısı (≤ cfg::MAX_PEAKS = 8).
   *
   * Sonuçlar peaks() ile erişilebilir (en güçlüden en zayıfa sıralı).
   */
  uint32_t detect(const float *magDb);
  /// Tespit edilen tepelerin dizisi.
  const SpectralPeak *peaks() const { return peaks_; }
  /// Kaç tepe bulundu?
  uint32_t peakCount() const { return peakCount_; }

private:
  SpectralPeak peaks_[cfg::MAX_PEAKS] = {}; // En fazla 8 tepe
  uint32_t peakCount_ = 0;

  /**
   * Parabolik interpolasyon.
   * @param magDb       Genlik dizisi.
   * @param k           Tepe bin indeksi.
   * @param[out] delta  Kesirli bin kayması (-0.5 .. +0.5).
   * @param[out] interpMag  İnterpolasyonlu genlik (dB).
   */
  static void parabolicInterp(const float *magDb, uint32_t k, float &delta,
                              float &interpMag);
};

#endif // PEAK_DETECTOR_HPP
