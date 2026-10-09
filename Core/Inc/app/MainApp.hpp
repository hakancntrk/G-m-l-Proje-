/**
 * @file    MainApp.hpp
 * @brief   Ana uygulama orkestratörü — durum makinesi (state machine).
 */
#ifndef MAIN_APP_HPP
#define MAIN_APP_HPP

// ── Kullanacağımız tüm modüllerin başlık dosyaları ──
#include "interfaces/IDisplay.hpp"
#include "interfaces/ILogSink.hpp"
#include "interfaces/ISampleSource.hpp"

#include "app/SpectrumView.hpp"
#include "dsp/PeakDetector.hpp"
#include "dsp/PitchEstimator.hpp"
#include "dsp/SpectrumAnalyzer.hpp"

#include "Configurer.hpp"

class MainApp {
public:
  MainApp(ISampleSource &sampler, IDisplay &display, ILogSink &log);
  /// Peripheral'ler init olduktan sonra bir kez çağır.
  /// Splash screen gösterir, sampler'ı başlatır.
  void init();
  /// Ana döngüden (while(true)) sürekli çağrılır.
  /// Non-blocking: iş yoksa hemen döner.
  void tick();

private:
  // ── Bağımlılıklar (referanslar) ──
  ISampleSource &sampler_; // Ses verisi kaynağı
  IDisplay &display_;      // Ekran
  ILogSink &log_;          // Log çıkışı

  // ── DSP modülleri (bu sınıfın üyeleri, otomatik oluşturulur) ──
  SpectrumAnalyzer spectrum_; // FFT + genlik
  PeakDetector peakDet_;      // Tepe tespiti
  PitchEstimator pitchEst_;   // Pitch tahmini
  SpectrumView view_;         // Ekran çizici

  // ── Durum değişkenleri ──
  uint32_t lastLogTick_; // Son log gönderilme zamanı (ms)
  uint32_t frameCount_;  // Kaç çerçeve işlendi (debug için)

  // Son pitch sonucu (RENDERING'den LOGGING'e aktarmak için saklanır)
  PitchResult lastPitch_;

  // ── Dahili fonksiyonlar ──
  void showSplashScreen();                  // Açılış ekranı çizimi
  void printStartupLog();                   // Başlangıç log metni
  void printStatusLog();                    // Anlık durum log metni

  void doProcessing(const uint16_t *frame); // DSP zincirini çalıştır
  void doLogging();                         // UART log gönder (throttled)
  uint32_t getTick() const;                 // HAL_GetTick() sarmalayıcı
};

#endif // MAIN_APP_HPP
