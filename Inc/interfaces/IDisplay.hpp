/**
 * @file    IDisplay.hpp
 * @brief   Ekran donanımı için arayüz (interface).
 */
#ifndef IDISPLAY_HPP
#define IDISPLAY_HPP

#include <cstdint>

class IDisplay {
public:
  /// Tüm ekranı tek bir renkle doldur (temizle).
  /// colour = RGB565 formatında renk kodu.  Varsayılan: 0x0000 = siyah.
  virtual void clear(uint16_t colour = 0x0000) = 0;
  /// Dolu dikdörtgen çiz (spektrum çubukları için kullanacağız).
  ///   (x, y) = sol üst köşe
  ///   w = genişlik,  h = yükseklik
  virtual void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        uint16_t colour) = 0;
  /// (x, y) konumuna yazı yaz.
  /// Font (yazı tipi) alt sınıfın içinde tanımlı (5×7 piksel).
  virtual void drawText(uint16_t x, uint16_t y, const char *text,
                        uint16_t colour) = 0;
};

#endif // IDISPLAY_HPP
