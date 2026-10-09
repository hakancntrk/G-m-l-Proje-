/**
 * @file    PitchEstimator.cpp
 * @brief   Harmonik skorlama ile temel frekans (f0) tahmini.
 */
#include "dsp/PitchEstimator.hpp"
#include <algorithm> // std::sort
#include <cmath>     // log2f, roundf, exp2f, fabs
#include <cstdio>    // snprintf

// ════════════════════════════════════════════════════════════════
// SABİTLER
// ════════════════════════════════════════════════════════════════

static const char *NOTE_NAMES[] = {"C",  "C#", "D",  "Eb", "E",  "F",
                                   "F#", "G",  "Ab", "A",  "Bb", "B"};

constexpr float INV_440 = 1.0f / 440.0f;
constexpr float INV_12 = 1.0f / 12.0f;

// Harmonik ağırlıkları (1/h): FPU bölme maliyetini sıfırlamak için
static constexpr float H_WEIGHTS[cfg::MAX_HARMONICS] = {
    1.0f, 1.0f / 2.0f, 1.0f / 3.0f, 1.0f / 4.0f, 1.0f / 5.0f, 1.0f / 6.0f};

// ════════════════════════════════════════════════════════════════
// FREKANS → NOTA DÖNÜŞÜMÜ
// ════════════════════════════════════════════════════════════════

void PitchEstimator::freqToNote(float freqHz, char *nameOut, size_t nameSize,
                                float &centOut) {
  if (freqHz <= 0.0f) {
    snprintf(nameOut, nameSize, "---");
    centOut = 0.0f;
    return;
  }

  // MIDI = 69 + 12 × log2(f / 440)
  const float midiFloat = 69.0f + 12.0f * log2f(freqHz * INV_440);
  const int midiNote = static_cast<int>(roundf(midiFloat));

  centOut = (midiFloat - static_cast<float>(midiNote)) * 100.0f;

  const int noteIndex = ((midiNote % 12) + 12) % 12;
  const int octave = (midiNote / 12) - 1;

  snprintf(nameOut, nameSize, "%s%d", NOTE_NAMES[noteIndex], octave);
}

// ════════════════════════════════════════════════════════════════
// SPEKTRUMDA RASTGELE FREKANSTA GENLİK OKUMA
// ════════════════════════════════════════════════════════════════

float PitchEstimator::magAtFreq(const float *magLin, float freqHz) {
  constexpr float invBinWidth = 1.0f / cfg::FREQ_BIN_WIDTH_HZ;
  const float binF = freqHz * invBinWidth;

  if (binF < 0.0f || binF >= static_cast<float>(cfg::HALF_FFT - 1)) {
    return 0.0f;
  }

  const auto binLow = static_cast<uint32_t>(binF);
  const float frac = binF - static_cast<float>(binLow);

  // FMA uyumlu lineer interpolasyon: a + frac * (b - a)
  return magLin[binLow] + frac * (magLin[binLow + 1] - magLin[binLow]);
}

// ════════════════════════════════════════════════════════════════
// ANA TAHMİN FONKSİYONU
// ════════════════════════════════════════════════════════════════

PitchResult PitchEstimator::estimate(const SpectralPeak *peaks,
                                     uint32_t peakCount, const float *magLin) {
  PitchResult result{};
  snprintf(result.noteName, sizeof(result.noteName), "---");

  float bestScore = 0.0f;
  float bestF0 = 0.0f;
  constexpr float nyquist = static_cast<float>(cfg::SAMPLE_RATE_HZ) * 0.5f;

  // ── Her tespit edilen tepeyi ve alt-harmonikleri dene ──
  for (uint32_t p = 0; p < peakCount; ++p) {
    const float candidate = peaks[p].frequencyHz;
    if (candidate < cfg::F0_MIN_HZ || candidate > cfg::F0_MAX_HZ)
      continue;

    const float candidates[] = {candidate, candidate * 0.5f,
                                candidate * (1.0f / 3.0f)};

    for (float f0c : candidates) {
      if (f0c < cfg::F0_MIN_HZ || f0c > cfg::F0_MAX_HZ)
        continue;

      float score = 0.0f;
      for (uint32_t h = 1; h <= cfg::MAX_HARMONICS; ++h) {
        const float hFreq = f0c * static_cast<float>(h);
        if (hFreq > nyquist)
          break;

        // FPU bölmesi yerine önceden hesaplanmış sabit çarpan
        score += magAtFreq(magLin, hFreq) * H_WEIGHTS[h - 1];
      }

      if (score > bestScore) {
        bestScore = score;
        bestF0 = f0c;
      }
    }
  }

  // ── Alt-oktav Doğrulaması (Sub-octave validation) ──
  if (bestF0 > 0.0f && bestScore > 1e-6f) {
    const float subF0 = bestF0 * 0.5f;
    if (subF0 >= cfg::F0_MIN_HZ) {
      const float magB = magAtFreq(magLin, bestF0);
      const float magSub = magAtFreq(magLin, subF0);
      const float mag1_5 = magAtFreq(magLin, bestF0 * 1.5f);

      if (magSub > (magB * 0.10f) && mag1_5 > (magB * 0.05f)) {
        bestF0 = subF0;
      }
    }
  } else {
    bestF0 = 0.0f;
  }

  // ── Zamansal kararlılık (Temporal stability) ──
  const float currentMidi = (bestF0 > 0.0f)
                                ? (69.0f + 12.0f * log2f(bestF0 * INV_440))
                                : cfg::NO_PITCH_MIDI;

  history_[histIdx_] = currentMidi;
  histIdx_ = (histIdx_ + 1u) % cfg::STABILITY_FRAMES;

  // Medyan hesabı için sıralama
  float sorted[cfg::STABILITY_FRAMES];
  for (uint32_t i = 0; i < cfg::STABILITY_FRAMES; ++i)
    sorted[i] = history_[i];
  std::sort(sorted, sorted + cfg::STABILITY_FRAMES);

  const float median = sorted[cfg::STABILITY_FRAMES / 2u];

  // Medyana yakın kareleri filtrele ve ortala
  uint32_t closeCount = 0;
  float closeSum = 0.0f;

  for (uint32_t i = 0; i < cfg::STABILITY_FRAMES; ++i) {
    const float val = history_[i];
    if (val > cfg::NO_PITCH_MIDI + 100.0f &&
        std::fabs(val - median) <= cfg::STABILITY_TOL_SEMITONES) {
      ++closeCount;
      closeSum += val;
    }
  }

  const bool currentOk =
      (currentMidi > cfg::NO_PITCH_MIDI + 100.0f) &&
      (std::fabs(currentMidi - median) <= cfg::STABILITY_TOL_SEMITONES);

  if (median > cfg::NO_PITCH_MIDI + 100.0f &&
      closeCount >= cfg::STABILITY_MIN_CLOSE && currentOk) {

    const float midiAvg = closeSum / static_cast<float>(closeCount);
    result.f0Hz = 440.0f * exp2f((midiAvg - 69.0f) * INV_12);
    freqToNote(result.f0Hz, result.noteName, sizeof(result.noteName),
               result.centDeviation);
    result.valid = true;
  }

  return result;
}
