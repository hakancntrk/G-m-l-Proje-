/**
 * @file    SpectrumView.cpp
 * @brief   Spektrum çubukları ve bilgi paneli çizimi.
 *
 */
#include "app/SpectrumView.hpp"
#include <cstdio>  // snprintf
#include <cstring> // strcmp, memcpy
#include <cmath>  // powf

// ════════════════════════════════════════════════════════════════
// SABİTLER
// ════════════════════════════════════════════════════════════════

// Çubukların dB aralığı: -80 dB (sessiz) ile 0 dB (en güçlü) arası
static constexpr float DB_MIN = -80.0f; // Bu değerin altı = ekranda 0 piksel
static constexpr float DB_MAX = 0.0f;   // Bu değer = ekranda 90 piksel

// ════════════════════════════════════════════════════════════════
// YAPICI FONKSİYON
// ════════════════════════════════════════════════════════════════

SpectrumView::SpectrumView(IDisplay &display) : display_(display) {}

// ════════════════════════════════════════════════════════════════
// YARDIMCI: dB → Piksel yüksekliği
// ════════════════════════════════════════════════════════════════

uint16_t SpectrumView::dbToBarHeight(float db) {
  // dB aralığını sınırla
  if (db < DB_MIN)
    db = DB_MIN;
  if (db > DB_MAX)
    db = DB_MAX;

  float norm = (db - DB_MIN) / (DB_MAX - DB_MIN);

  return static_cast<uint16_t>(norm * cfg::SPEC_AREA_H);
}

// ════════════════════════════════════════════════════════════════
// YARDIMCI: Bir sütunun [from, to] yükseklik aralığını renk bantlarıyla çiz
// ════════════════════════════════════════════════════════════════

void SpectrumView::drawBarSegment(uint16_t col, uint16_t from, uint16_t to) {
  const uint16_t H = cfg::SPEC_AREA_H;
  // Bant sınırları (yükseklik, piksel): 0-%50 yeşil, %50-80 sarı, %80-100 kırmızı
  const uint16_t edge[4] = {0, static_cast<uint16_t>(H / 2),
                            static_cast<uint16_t>((H * 4) / 5), H};
  const uint16_t colour[3] = {cfg::COL_BAR_LOW, cfg::COL_BAR_MID, cfg::COL_BAR_HIGH};

  for (uint8_t b = 0; b < 3; ++b) {
    const uint16_t lo = (from > edge[b]) ? from : edge[b];
    const uint16_t hi = (to < edge[b + 1]) ? to : edge[b + 1];
    if (hi > lo) {
      display_.fillRect(cfg::SPEC_AREA_X + col, cfg::SPEC_AREA_Y + H - hi, 1,
                        hi - lo, colour[b]);
    }
  }
}

// ════════════════════════════════════════════════════════════════
// SPEKTRUM ÇUBUKLARI ÇİZİMİ
// ════════════════════════════════════════════════════════════════

void SpectrumView::drawSpectrumBars(const float *magDb) {
  const uint32_t numBins = cfg::HALF_FFT;    // FFT bin sayısı (N/2)
  const uint16_t numCols = cfg::SPEC_AREA_W; // 160 piksel sütun

  // Her piksel sütunu için:
  for (uint16_t col = 0; col < numCols; ++col) {

    // ── Logaritmik Frekans Eşlemesi ──
    // Claude'un önerisiyle ekranı daha iyi kullanmak için 50Hz - 8000Hz aralığı.
    const float minFreq = 50.0f;
    const float maxFreq = 8000.0f;
    const float ratio = maxFreq / minFreq;

    float freqStart = minFreq * powf(ratio, static_cast<float>(col) / numCols);
    float freqEnd = minFreq * powf(ratio, static_cast<float>(col + 1) / numCols);

    float binF_start = freqStart / cfg::FREQ_BIN_WIDTH_HZ;
    float binF_end = freqEnd / cfg::FREQ_BIN_WIDTH_HZ;

    uint32_t binStart = static_cast<uint32_t>(binF_start);
    uint32_t binEnd = static_cast<uint32_t>(binF_end);

    if (binEnd > numBins) binEnd = numBins;

    float maxDb = DB_MIN;

    // Eğer bir sütun birden çok bin'i kapsıyorsa (yüksek frekanslar), maksimumunu al
    if (binEnd > binStart) {
        for (uint32_t b = binStart; b < binEnd; ++b) {
            if (magDb[b] > maxDb) maxDb = magDb[b];
        }
    } 
    // Eğer bir sütun bir bin'den daha dar ise (düşük frekanslar), lineer interpolasyon yap
    // Bu sayede "merdiven" efekti yerine yumuşak geçişli çubuklar elde ederiz.
    else {
        uint32_t b0 = binStart;
        uint32_t b1 = (b0 + 1 < numBins) ? (b0 + 1) : b0;
        
        // binF_start'ın kesirli kısmı üzerinden interpolasyon
        float frac = binF_start - static_cast<float>(b0);
        float interpDb = magDb[b0] * (1.0f - frac) + magDb[b1] * frac;
        maxDb = interpDb;
    }

    // ── Çubuk yüksekliğini hesapla ──
    const uint16_t barH = dbToBarHeight(maxDb);

    // ── Artımlı çizim: yalnızca önceki kareye göre değişen pikselleri gönder ──
    const uint16_t prev = prevH_[col];
    if (barH > prev) {
      // Çubuk uzadı → sadece yeni eklenen satırları çiz
      drawBarSegment(col, prev, barH);
    } else if (barH < prev) {
      // Çubuk kısaldı → sadece artan kısmı siyahla sil
      display_.fillRect(cfg::SPEC_AREA_X + col,
                        cfg::SPEC_AREA_Y + cfg::SPEC_AREA_H - prev, 1,
                        prev - barH, cfg::COL_BG);
    }
    prevH_[col] = barH;
  }
}

// ════════════════════════════════════════════════════════════════
// BİLGİ PANELİ ÇİZİMİ
// ════════════════════════════════════════════════════════════════

void SpectrumView::drawInfoLine(uint8_t idx, uint16_t y, const char *text,
                                uint16_t colour) {
  static_assert(INFO_CHARS == 26, "snprintf biçimindeki 26 ile eşleşmeli");

  // Satırı sabit genişliğe boşlukla doldur: eski yazının artıkları kendiliğinden silinir
  char line[INFO_CHARS + 1];
  snprintf(line, sizeof(line), "%-26.26s", text);

  if (strcmp(line, prevLine_[idx]) == 0)
    return; // Metin değişmedi → ekrana hiçbir şey gönderme

  display_.drawText(2, y, line, colour);
  memcpy(prevLine_[idx], line, sizeof(line));
}

void SpectrumView::drawInfoPanel(const SpectralPeak *peaks, uint32_t peakCount,
                                 const PitchResult &pitch) {
  // Alt bölgeyi ve ayırıcı çizgiyi yalnızca BİR KEZ çiz
  if (!panelDrawn_) {
    display_.fillRect(0, cfg::SPEC_AREA_H, cfg::DISPLAY_WIDTH,
                      cfg::DISPLAY_HEIGHT - cfg::SPEC_AREA_H, cfg::COL_BG);
    display_.fillRect(0, cfg::SPEC_AREA_H, cfg::DISPLAY_WIDTH, 1, cfg::COL_LABEL);
    panelDrawn_ = true;
  }

  char buf[32];

  // ── Satır 1: En güçlü tepe bilgisi ──
  if (peakCount > 0) {
    snprintf(buf, sizeof(buf), "Pk:%.0fHz %.0fdB", peaks[0].frequencyHz,
             peaks[0].magnitudeDb);
  } else {
    snprintf(buf, sizeof(buf), "Pk: --- Hz");
  }
  drawInfoLine(0, cfg::INFO_AREA_Y, buf, cfg::COL_TEXT);

  // ── Satır 2: Pitch tahmini (f0 + nota + cent) ──
  if (pitch.valid) {
    snprintf(buf, sizeof(buf), "f0:%.0fHz %s %+.0fc", pitch.f0Hz, pitch.noteName,
             pitch.centDeviation);
  } else {
    snprintf(buf, sizeof(buf), "f0: --- Hz");
  }
  drawInfoLine(1, cfg::INFO_AREA_Y + 10, buf, cfg::COL_TEXT);

  // ── Satır 3: Sistem bilgisi (sabit; yalnızca ilk karede çizilir) ──
  snprintf(buf, sizeof(buf), "fs=%lu N=%lu",
           static_cast<unsigned long>(cfg::SAMPLE_RATE_HZ),
           static_cast<unsigned long>(cfg::FFT_SIZE));
  drawInfoLine(2, cfg::INFO_AREA_Y + 20, buf, cfg::COL_LABEL);
}

// ════════════════════════════════════════════════════════════════
// ANA RENDER FONKSİYONU
// ════════════════════════════════════════════════════════════════

void SpectrumView::render(const float *magDb, const SpectralPeak *peaks,
                          uint32_t peakCount, const PitchResult &pitch) {
  drawSpectrumBars(magDb);                // Üst kısım: çubuklar
  drawInfoPanel(peaks, peakCount, pitch); // Alt kısım: yazılar
}
