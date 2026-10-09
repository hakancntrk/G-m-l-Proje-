/**
 * @file    UartLogSink.hpp
 * @brief   ILogSink arayüzünü USART2 üzerinden gerçekleştiren sınıf.
 *
 * Nucleo-64 kartında USART2 (PA2/PA3) doğrudan ST-Link'e bağlıdır.
 * USB kablonuzu taktığınızda bilgisayarda bir COM portu açılır.
 * Putty, Tera Term veya Arduino Serial Monitor ile okuyabilirsiniz.
 * Baud rate: 38400, 8N1 (8 bit, parity yok, 1 stop bit).
 */
#ifndef UART_LOG_SINK_HPP
#define UART_LOG_SINK_HPP

#include "interfaces/ILogSink.hpp"

#include "stm32f3xx_hal.h"

class UartLogSink : public ILogSink {
public:
  explicit UartLogSink(UART_HandleTypeDef *huart);
  /// ILogSink'ten: text stringini UART üzerinden gönder.
  void write(const char *text) override;

private:
  UART_HandleTypeDef *huart_; // UART handle pointer'ı
};

#endif // UART_LOG_SINK_HPP
