/**
 * @file    ILogSink.hpp
 * @brief   Loglama (seri port çıktısı) için arayüz.
 *
 * En basit interface bu — tek bir fonksiyon: write().
 * UART ile gerçek donanıma yazar (UartLogSink).
 */
#ifndef ILOG_SINK_HPP
#define ILOG_SINK_HPP

class ILogSink {
public:
  /// Null-terminated (sonu '\0' olan) bir string'i log çıkışına yaz.
  /// Gerçek uygulamada: HAL_UART_Transmit ile seri porttan gönderir.
  virtual void write(const char *text) = 0;
};

#endif // ILOG_SINK_HPP
