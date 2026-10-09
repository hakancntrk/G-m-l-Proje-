/**
 * @file    UartLogSink.cpp
 * @brief   UART loglama implementasyonu.
 */
#include "hal/UartLogSink.hpp"

#include <cstring> // strlen() için

UartLogSink::UartLogSink(UART_HandleTypeDef *huart) : huart_(huart) {}

void UartLogSink::write(const char *text) {
  if (!text)
    return;
  // String'in uzunluğunu öğren
  uint16_t len = static_cast<uint16_t>(strlen(text));
  // HAL_UART_Transmit: Blocking (engelleyici) mod.
  // Tüm byte'lar gönderilene kadar bekler.
  // Son parametre timeout (ms). HAL_MAX_DELAY = sonsuza kadar bekle.
  HAL_UART_Transmit(huart_, reinterpret_cast<const uint8_t *>(text), len,
                    HAL_MAX_DELAY);
}
