/**
 * @file    ISampleSource.hpp
 * @brief   Ses örneklerini sağlayan birimlerin ortak arayüzü (interface).
 *
 */
#ifndef ISAMPLE_SOURCE_HPP
#define ISAMPLE_SOURCE_HPP

#include <cstdint> // uint16_t, uint32_t için

class ISampleSource {
public:
  /// Sürekli örneklemeyi başlat (ADC + DMA'yı çalıştır).
  virtual bool start() = 0;

  /// Bir çerçeve (frame = FFT_SIZE adet örnek) hazır mı?
  /// true dönerse frame() ile veriyi alabilirsin.
  virtual bool frameReady() const = 0;

  /// Hazır olan çerçevenin bellekteki adresini döndürür.
  /// veri en fazla bir sonraki yarı-transfer'e (≈ 42,7 ms) kadar geçerli; hemen
  /// kopyala.
  virtual const uint16_t *frame() const = 0;

  /// Çerçeveyi "aldım" diye işaretle.
  /// DMA o bellek alanını tekrar kullanabilsin.
  virtual void release() = 0;

  /// Üretilip işlenemeden atlanan çerçeve sayısı (varsayılan: 0).
  /// Gerçek örnekleyici bunu override eder; mock'lar değiştirilmek zorunda
  /// değil.
  virtual uint32_t droppedFrames() const { return 0; }
};

#endif // ISAMPLE_SOURCE_HPP
