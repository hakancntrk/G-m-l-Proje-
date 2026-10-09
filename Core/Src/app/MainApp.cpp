/**
 * @file    MainApp.cpp
 * @brief   Ana uygulama durum makinesi implementasyonu.
 *
 * tick() fonksiyonu main loop'tan çağrılır.
 * Her çağrıda mevcut duruma göre bir iş yapar ve bir sonraki duruma geçer.
 */
#include "app/MainApp.hpp"
#include <cstdio>  // snprintf
#include <cstring> // memset

#include "stm32f3xx_hal.h" // HAL_GetTick, HAL_Delay

// ════════════════════════════════════════════════════════════════
// YAPICI FONKSİYON
// ════════════════════════════════════════════════════════════════

MainApp::MainApp(ISampleSource &sampler, IDisplay &display,
                         ILogSink &log)
    : sampler_(sampler) // Referansları başlat
      ,
      display_(display), log_(log),
      spectrum_() // DSP modülleri (default constructor)
      ,
      peakDet_(), pitchEst_(),
      view_(display) // SpectrumView ekran referansını alır
      ,
      lastLogTick_(0), frameCount_(0),
      lastPitch_{} // Sıfırla ({} = zero-initialize)
{}

// ════════════════════════════════════════════════════════════════
// PUBLIC API (Arayüz Metotları)
// ════════════════════════════════════════════════════════════════

void MainApp::init() {
  showSplashScreen(); // Açılış ekranını göster
  printStartupLog();  // Başlangıç loglarını yazdır

  HAL_Delay(1500); // 1.5 saniye splash screen göster

  // ── Ekranı temizle (splash'ı sil) ──
  display_.clear(cfg::COL_BG);

  // ── Örneklemeyi başlat ──
  if (!sampler_.start()) {
    log_.write("ADC/DMA start FAILED\r\n");
  }
}

void MainApp::tick() {
  if (!sampler_.frameReady())
    return;
  doProcessing(sampler_.frame());
  sampler_.release();
  view_.render(spectrum_.magnitudeDb(), peakDet_.peaks(), peakDet_.peakCount(),
               lastPitch_);

  doLogging();
}

// ════════════════════════════════════════════════════════════════
// CORE LOGIC (Çekirdek İş Akışları)
// ════════════════════════════════════════════════════════════════

void MainApp::doProcessing(const uint16_t *frame) {
  // Adım 1: Ham ADC verisini FFT'ye sok, genlik spektrumunu hesapla
  spectrum_.process(frame);

  // Adım 2: Genlik spektrumunda en güçlü tepeleri bul
  peakDet_.detect(spectrum_.magnitudeDb());

  // Adım 3: Tepelerden temel frekansı (f0) tahmin et
  lastPitch_ = pitchEst_.estimate(
      peakDet_.peaks(),        // Bulunan tepeler
      peakDet_.peakCount(),    // Tepe sayısı
      spectrum_.magnitudeLin() // Lineer genlik (interpolasyon için)
  );

  ++frameCount_; // Debug: kaç çerçeve işlendi
}

void MainApp::doLogging() {
  uint32_t now = getTick(); // Şu anki zaman (ms)

  // Son logdan bu yana 500 ms geçmediyse → atla
  if (now - lastLogTick_ < cfg::LOG_INTERVAL_MS)
    return;
  lastLogTick_ = now;

  printStatusLog();
}

// ════════════════════════════════════════════════════════════════
// PRIVATE HELPERS (Yardımcı Fonksiyonlar)
// ════════════════════════════════════════════════════════════════

void MainApp::showSplashScreen() {
  display_.clear(cfg::COL_BG); // Ekranı siyahla temizle
  display_.drawText(20, 30, "Audio Spectrum", cfg::COL_LABEL);
  display_.drawText(30, 45, "Analyzer", cfg::COL_LABEL);
  display_.drawText(15, 70, "STM32F303RE", cfg::COL_TEXT);
  char info[32];
  snprintf(info, sizeof(info), "fs=%lu N=%lu",
           static_cast<unsigned long>(cfg::SAMPLE_RATE_HZ),
           static_cast<unsigned long>(cfg::FFT_SIZE));
  display_.drawText(15, 85, info, cfg::COL_TEXT);
}

void MainApp::printStartupLog() {
  log_.write("\r\n=== Audio Spectrum Analyzer ===\r\n");
  char banner[80];
  snprintf(banner, sizeof(banner),
           "STM32F303RE | fs=%lu | N=%lu | CMSIS-DSP FFT\r\n\r\n",
           static_cast<unsigned long>(cfg::SAMPLE_RATE_HZ),
           static_cast<unsigned long>(cfg::FFT_SIZE));
  log_.write(banner);
}

void MainApp::printStatusLog() {
  char buf[128];

  if (peakDet_.peakCount() > 0) {
    const SpectralPeak &p = peakDet_.peaks()[0];

    if (lastPitch_.valid) {
      snprintf(buf, sizeof(buf),
               "[%lu] Peak: %.1f Hz (%.1f dB) | f0: %.1f Hz %s %+.0fc | drop:%lu\r\n",
               static_cast<unsigned long>(frameCount_), p.frequencyHz,
               p.magnitudeDb, lastPitch_.f0Hz, lastPitch_.noteName,
               lastPitch_.centDeviation,
               static_cast<unsigned long>(sampler_.droppedFrames()));
    } else {
      snprintf(buf, sizeof(buf),
               "[%lu] Peak: %.1f Hz (%.1f dB) | f0: --- | drop:%lu\r\n",
               static_cast<unsigned long>(frameCount_), p.frequencyHz,
               p.magnitudeDb,
               static_cast<unsigned long>(sampler_.droppedFrames()));
    }
  } else {
    snprintf(buf, sizeof(buf), "[%lu] No signal | drop:%lu\r\n",
             static_cast<unsigned long>(frameCount_),
             static_cast<unsigned long>(sampler_.droppedFrames()));
  }

  log_.write(buf);
}

uint32_t MainApp::getTick() const {
  return HAL_GetTick(); // ms cinsinden sistem zamanı (SysTick'ten)
}
