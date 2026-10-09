/**
 * @file    SpectrumView.hpp
 * @brief   Spektrum çubukları ve bilgi panelini ekrana çizen modül.
 *
 * Ekran düzeni (160×128 piksel, yatay):
 *
 *   ┌──────────────────────────────────────┐
 *   │  Spektrum çubukları (90 piksel)      │  Log frekans ekseni 50 Hz-8 kHz
 *   │  ▓▓ ▓▓▓ ▓▓ ▓▓▓▓▓ ▓▓ ▓▓             │  Renk: yeşil→sarı→kırmızı
 *   ├──────────────────────────────────────┤
 *   │  ──── ayırıcı çizgi ────            │  1 piksel, cyan
 *   ├──────────────────────────────────────┤
 *   │  Pk: 440Hz -12dB                    │  En güçlü tepe bilgisi
 *   │  f0: 440Hz A4 +2c                   │  Pitch tahmini + nota
 *   │  fs=48000 N=2048                    │  Sistem bilgisi
 *   └──────────────────────────────────────┘
 */
#ifndef SPECTRUM_VIEW_HPP
#define SPECTRUM_VIEW_HPP

#include "interfaces/IDisplay.hpp"     // Ekran arayüzü
#include "dsp/PeakDetector.hpp"        // SpectralPeak struct'ı
#include "dsp/PitchEstimator.hpp"      // PitchResult struct'ı
#include "Configurer.hpp"

class SpectrumView {
public:
    /// Yapıcı: Referans olarak display alır.
    explicit SpectrumView(IDisplay& display);

    /**
     * Tam bir kare çiz (spektrum + bilgi paneli).
     *
     * @param magDb       dB genlik spektrumu (cfg::HALF_FFT eleman).
     * @param peaks       Tespit edilen tepeler.
     * @param peakCount   Tepe sayısı.
     * @param pitch       Pitch tahmin sonucu.
     */
    void render(const float* magDb,
                const SpectralPeak* peaks, uint32_t peakCount,
                const PitchResult& pitch);

private:
    IDisplay& display_;    // Ekran referansı (ömür boyu geçerli)

    void drawSpectrumBars(const float* magDb);
    void drawInfoPanel(const SpectralPeak* peaks, uint32_t peakCount,
                       const PitchResult& pitch);

    /// dB değerini çubuk yüksekliğine (0..90 piksel) dönüştür.
    static uint16_t dbToBarHeight(float db);

    /// Bir sütunda [from, to] yükseklik aralığını renk bantlarına bölerek çiz
    /// (alt %50 yeşil, %50-80 sarı, üstü kırmızı).
    void drawBarSegment(uint16_t col, uint16_t from, uint16_t to);

    /// Bilgi paneli satırını yalnızca metni değiştiyse çiz (26 karaktere boşlukla doldurur).
    void drawInfoLine(uint8_t idx, uint16_t y, const char* text, uint16_t colour);

    static constexpr uint8_t INFO_CHARS = 26;   // (160 px - 2 px kenar boşluğu) / 6 px karakter

    // ── Artımlı çizim durumu: yalnızca değişen pikseller ekrana gönderilir ──
    uint16_t prevH_[cfg::SPEC_AREA_W] = {};     // Önceki karedeki çubuk yükseklikleri
    char     prevLine_[3][INFO_CHARS + 1] = {}; // Bilgi satırlarının önceki metni
    bool     panelDrawn_ = false;               // Ayırıcı çizgi/alt bölge bir kez temizlendi mi
};

#endif // SPECTRUM_VIEW_HPP
