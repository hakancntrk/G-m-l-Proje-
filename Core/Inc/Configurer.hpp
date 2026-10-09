/**
 * @file    Configurer.hpp
 * @brief   Projenin tüm ayarlarının tek bir yerde toplandığı dosya.
 */
#ifndef CONFIGURER_HPP // Bu dosya birden fazla kez #include edilirse
#define CONFIGURER_HPP // sadece bir kez derlenmesini sağlar (include guard).

#include <cstdint> // uint32_t, uint16_t gibi sabit boyutlu tipleri sağlar

namespace cfg {
// ═══════════════════════════════════════════════════════════════
//  ÖRNEKLEME (SAMPLING) AYARLARI
// ═══════════════════════════════════════════════════════════════
constexpr uint32_t SAMPLE_RATE_HZ = 48000u; // TIM6 48kHz'e ayarlanmalı (Prescaler: 0, Period: 1499)
constexpr uint32_t FFT_SIZE = 2048u;        // Çözünürlüğü korumak için 2048'e çıkarıldı
constexpr uint32_t HALF_FFT = FFT_SIZE / 2u;
constexpr float FREQ_BIN_WIDTH_HZ =
    static_cast<float>(SAMPLE_RATE_HZ) / FFT_SIZE;
// ═══════════════════════════════════════════════════════════════
//  ADC (Analog-Dijital Çevirici) AYARLARI
// ═══════════════════════════════════════════════════════════════
constexpr uint32_t ADC_RESOLUTION_BITS = 12u;
constexpr float ADC_MAX_VALUE =
    static_cast<float>((1u << ADC_RESOLUTION_BITS) - 1u);
// ═══════════════════════════════════════════════════════════════
//  TEPE TESPİTİ (PEAK DETECTION) AYARLARI
// ═══════════════════════════════════════════════════════════════
constexpr uint32_t MAX_PEAKS = 8u; // En fazla kaç tepe noktası bulacağız
constexpr float PEAK_THRESHOLD_DB =
    -40.0f; // Bu dB'nin altındaki tepeleri yoksay
constexpr uint32_t MIN_PEAK_BIN =
    2u; // İlk 2 bin'i atla (DC bileşeni + gürültü)
constexpr uint32_t MAX_PEAK_BIN = HALF_FFT - 2u; // k+1 erişimi için
// ═══════════════════════════════════════════════════════════════
//  PİTCH (TEMEL FREKANS) TAHMİNİ AYARLARI
// ═══════════════════════════════════════════════════════════════
constexpr float F0_MIN_HZ = 65.0f;     // 50 Hz şebeke gürültüsünden kaçınmak için 65'e çıkarıldı
constexpr float F0_MAX_HZ = 4200.0f;   // En yüksek temel frekans (~C8 notası)
constexpr uint32_t MAX_HARMONICS = 6u; // Kaç harmonik kontrol edilecek
// ── Zamansal kararlılık (valid) kuralı ──
constexpr uint32_t STABILITY_FRAMES = 5u;        // Geçmişte tutulan kare sayısı
constexpr uint32_t STABILITY_MIN_CLOSE = 4u;     // Bunlardan en az kaçı medyana yakın olmalı
constexpr float STABILITY_TOL_SEMITONES = 0.30f; // "Yakın" = ±30 cent
constexpr float NO_PITCH_MIDI = -1000.0f;        // "O karede pitch yok" işareti
// ═══════════════════════════════════════════════════════════════
//  EKRAN (ST7735S TFT) AYARLARI
// ═══════════════════════════════════════════════════════════════
constexpr uint16_t DISPLAY_WIDTH = 160u;
constexpr uint16_t DISPLAY_HEIGHT = 128u;
constexpr uint16_t SPEC_AREA_X = 0u;            // Sol kenar
constexpr uint16_t SPEC_AREA_Y = 0u;            // Üst kenar
constexpr uint16_t SPEC_AREA_W = DISPLAY_WIDTH; // Tam genişlik = 160
constexpr uint16_t SPEC_AREA_H = 90u;           // 90 piksel yükseklik
constexpr uint16_t INFO_AREA_Y = 94u; // Çubukların 4 piksel altında başlar
// ───────── Renk tanımları (RGB565 formatı) ─────────
// RGB565: 5 bit kırmızı, 6 bit yeşil, 5 bit mavi = 16 bit/piksel
constexpr uint16_t COL_BG = 0x0000;       // Siyah (arkaplan)
constexpr uint16_t COL_BAR_LOW = 0x07E0;  // Yeşil  (düşük seviye çubuklar)
constexpr uint16_t COL_BAR_MID = 0xFFE0;  // Sarı   (orta seviye çubuklar)
constexpr uint16_t COL_BAR_HIGH = 0xF800; // Kırmızı (yüksek seviye çubuklar)
constexpr uint16_t COL_TEXT = 0xFFFF;     // Beyaz  (yazılar)
constexpr uint16_t COL_LABEL = 0x07FF;    // Cyan   (etiketler)
// ═══════════════════════════════════════════════════════════════
//  LOGLAMA AYARLARI
// ═══════════════════════════════════════════════════════════════
constexpr uint32_t LOG_INTERVAL_MS = 500u;

} // namespace cfg

#endif // CONFIGURER_HPP
