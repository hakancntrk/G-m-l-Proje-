#ifndef ST7735_DISPLAY_HPP
#define ST7735_DISPLAY_HPP

#include "interfaces/IDisplay.hpp"
#include "Configurer.hpp"
#include "stm32f3xx_hal.h"

/**
 * @brief ST7735 SPI komut setini barındıran namespace.
 * Ekrana komut gönderirken okumayı kolaylaştırmak için tanımlanmıştır.
 */
namespace ST7735_Cmd {
    enum : uint8_t {
        SwReset     = 0x01, // Yazılımsal sıfırlama (Software Reset)
        SleepOut    = 0x11, // Uyku modundan çıkış (Sleep Out)
        NormalOn    = 0x13, // Normal görüntü modu (Normal Display ON)
        InvertOff   = 0x20, // Renk terslemeyi kapat (Display Inversion OFF)
        DisplayOn   = 0x29, // Ekranı aç (Display ON)
        ColAddrSet  = 0x2A, // Sütun adres aralığı (Column Address Set)
        RowAddrSet  = 0x2B, // Satır adres aralığı (Row Address Set)
        MemWrite    = 0x2C, // Belleğe yazma (Memory Write)
        MemDataCtrl = 0x36, // Ekran yönü/bellek kontrolü (MADCTL)
        ColMod      = 0x3A, // Renk formatı ayarı (16-bit vs.)
        FrmCtrl1    = 0xB1, // Frame Rate Control (Normal Mode)
        FrmCtrl2    = 0xB2, // Frame Rate Control (Idle Mode)
        FrmCtrl3    = 0xB3, // Frame Rate Control (Partial Mode)
        InvCtrl     = 0xB4, // Tersleme (Inversion) Kontrolü
        PwrCtrl1    = 0xC0, // Güç Kontrolü 1
        PwrCtrl2    = 0xC1, // Güç Kontrolü 2
        PwrCtrl3    = 0xC2, // Güç Kontrolü 3
        VcomCtrl1   = 0xC5, // VCOM Voltaj Kontrolü
        GammaPos    = 0xE0, // Pozitif Gamma Düzeltmesi
        GammaNeg    = 0xE1  // Negatif Gamma Düzeltmesi
    };
}

/**
 * @brief ST7735S TFT Ekran sürücü sınıfı.
 * IDisplay arayüzünü uygular. Çizim performansı için SPI block-write kullanır.
 */
class St7735Display : public IDisplay {
public:
    explicit St7735Display(SPI_HandleTypeDef* hspi);

    /// Ekranı başlatır (SPI, Pin Reset ve ST7735 başlatma dizisi)
    void init();

    // -- IDisplay Arayüzü Fonksiyonları --
    
    /// Tüm ekranı belirtilen renk ile doldurur.
    void clear(uint16_t colour = 0x0000) override;
    
    /// Blok olarak hızlı dikdörtgen çizer (Chunked SPI transfer).
    void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour) override;
    
    /// Belirtilen koordinata metin yazar.
    void drawText(uint16_t x, uint16_t y, const char* text, uint16_t colour) override;

private:
    SPI_HandleTypeDef* hspi_;

    // -- Yüksek Seviye Çizim Yardımcıları --
    void drawChar(uint16_t x, uint16_t y, char c, uint16_t colour);

    // -- Düşük Seviye SPI ve Kontrol Yardımcıları --
    void writeCommand(uint8_t cmd);
    void writeData(const uint8_t* data, uint16_t len);
    void writeData8(uint8_t data);
    void writeData16(uint16_t data);
    void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    // -- Donanım Pin Kontrolleri --
    void pinDC(bool level);
    void pinCS(bool level);
    void pinRST(bool level);
};

#endif // ST7735_DISPLAY_HPP
