#include "hal/St7735Display.hpp"
#include "hal/St7735Font.hpp"
#include <cstring>

// -----------------------------------------------------------------------------
// Donanım Pin Tanımları (Nucleo-F303RE)
// DC (Data/Command), RST (Reset), CS (Chip Select)
// -----------------------------------------------------------------------------
#define DC_PORT  GPIOB
#define DC_PIN   GPIO_PIN_0
#define RST_PORT GPIOB
#define RST_PIN  GPIO_PIN_1
#define CS_PORT  GPIOB
#define CS_PIN   GPIO_PIN_2

St7735Display::St7735Display(SPI_HandleTypeDef* hspi)
    : hspi_(hspi)
{}

// ═════════════════════════════════════════════════════════════════════════════
// PUBLIC API (Uygulama Katmanı Fonksiyonları)
// ═════════════════════════════════════════════════════════════════════════════

void St7735Display::init()
{
    // 1. Donanımsal Sıfırlama
    pinRST(false); HAL_Delay(50);
    pinRST(true);  HAL_Delay(50);

    // 2. Yazılımsal Sıfırlama ve Uyandırma
    writeCommand(ST7735_Cmd::SwReset);
    HAL_Delay(150); // Datasheet'e göre reset sonrası bekleme zorunludur

    writeCommand(ST7735_Cmd::SleepOut);
    HAL_Delay(150);

    // 3. Frame Rate (Ekran Yenileme) Ayarları
    writeCommand(ST7735_Cmd::FrmCtrl1);
    { uint8_t d[] = {0x01, 0x2C, 0x2D}; writeData(d, 3); }

    writeCommand(ST7735_Cmd::FrmCtrl2);
    { uint8_t d[] = {0x01, 0x2C, 0x2D}; writeData(d, 3); }

    writeCommand(ST7735_Cmd::FrmCtrl3);
    { uint8_t d[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D}; writeData(d, 6); }

    // 4. Güç ve Renk Tersleme (Inversion) Ayarları
    writeCommand(ST7735_Cmd::InvCtrl);
    writeData8(0x07);

    writeCommand(ST7735_Cmd::PwrCtrl1);
    { uint8_t d[] = {0xA2, 0x02, 0x84}; writeData(d, 3); }

    writeCommand(ST7735_Cmd::PwrCtrl2);
    writeData8(0xC5);

    writeCommand(ST7735_Cmd::PwrCtrl3);
    { uint8_t d[] = {0x0A, 0x00}; writeData(d, 2); }

    writeCommand(ST7735_Cmd::VcomCtrl1);
    writeData8(0x0E);

    writeCommand(ST7735_Cmd::InvertOff);

    // 5. Ekran Yönü: Yatay Çözünürlük (Landscape 160x128)
    writeCommand(ST7735_Cmd::MemDataCtrl);
    writeData8(0x60); // MV=1 (Satır/Sütun değişimi), MX=1 (X Eksenini Yansıt)

    // 6. Renk Formatı: 16-bit (RGB565)
    writeCommand(ST7735_Cmd::ColMod);
    writeData8(0x05);

    // 7. Gamma Düzeltmeleri (Renk doğruluğu için)
    writeCommand(ST7735_Cmd::GammaPos);
    {
        uint8_t d[] = {0x02,0x1C,0x07,0x12,0x37,0x32,0x29,0x2D,
                       0x29,0x25,0x2B,0x39,0x00,0x01,0x03,0x10};
        writeData(d, 16);
    }
    writeCommand(ST7735_Cmd::GammaNeg);
    {
        uint8_t d[] = {0x03,0x1D,0x07,0x06,0x2E,0x2C,0x29,0x2D,
                       0x2E,0x2E,0x37,0x3F,0x00,0x00,0x02,0x10};
        writeData(d, 16);
    }

    // 8. Ekranı Aktif Et
    writeCommand(ST7735_Cmd::NormalOn);
    HAL_Delay(10);
    writeCommand(ST7735_Cmd::DisplayOn);
    HAL_Delay(100);

    // Rastgele çöpleri temizle (Tamamen Siyah)
    clear(0x0000); 
}

void St7735Display::clear(uint16_t colour)
{
    // Tüm ekran sınırları kadar dikdörtgen çizimi yapar
    fillRect(0, 0, cfg::DISPLAY_WIDTH, cfg::DISPLAY_HEIGHT, colour);
}

void St7735Display::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour)
{
    // Sınır (Boundary) kontrolleri
    if (w == 0 || h == 0) return;
    if (x + w > cfg::DISPLAY_WIDTH)  w = cfg::DISPLAY_WIDTH  - x;
    if (y + h > cfg::DISPLAY_HEIGHT) h = cfg::DISPLAY_HEIGHT - y;

    // 1. Ekran denetleyicisine yazılacak alanı bildir (Sadece 1 kere)
    setAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t hi = colour >> 8;
    uint8_t lo = colour & 0xFF;
    
    // 2. Performans için SPI yazma tamponu (128 Byte = 64 Piksel)
    // Bu yöntem drawPixel() çağırmaya göre devasa bir FPS artışı sağlar.
    uint8_t buf[128];
    for (int i = 0; i < 64; ++i) {
        buf[i * 2] = hi;
        buf[i * 2 + 1] = lo;
    }

    uint32_t totalPixels = static_cast<uint32_t>(w) * h;
    pinDC(true);  // Veri Modu
    pinCS(false); // Ekranı Seç
    
    // 3. Tüm pikselleri paketler (chunk) halinde gönder
    while (totalPixels > 0) {
        uint16_t chunk = (totalPixels > 64) ? 64 : totalPixels;
        HAL_SPI_Transmit(hspi_, buf, chunk * 2, HAL_MAX_DELAY);
        totalPixels -= chunk;
    }
    pinCS(true); // İletişim bitti
}

void St7735Display::drawText(uint16_t x, uint16_t y, const char* text, uint16_t colour)
{
    // String bitene kadar (null terminator) harfleri yan yana çiz
    while (*text) {
        drawChar(x, y, *text, colour);
        x += 6; // 5 piksel karakter genişliği + 1 piksel harf arası boşluk
        ++text;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// PRIVATE HELPERS (Düşük Seviye Kontrol Fonksiyonları)
// ═════════════════════════════════════════════════════════════════════════════

void St7735Display::drawChar(uint16_t x, uint16_t y, char c, uint16_t colour)
{
    // Desteklenmeyen karakterler için '? ' göster
    if (c < 0x20 || c > 0x7E) c = '?';

    // Karakterin font dizisindeki başlangıç adresini bul
    const uint8_t* glyph = &Font::font5x7[(c - 0x20) * 5];
    uint8_t buf[5 * 7 * 2]; // 5x7 piksellik kutu, her piksel 2 byte
    
    uint8_t hi = colour >> 8;
    uint8_t lo = colour & 0xFF;
    uint8_t bgHi = cfg::COL_BG >> 8;
    uint8_t bgLo = cfg::COL_BG & 0xFF;

    // Bitmap verisini renk kodlarına çevir
    for (uint8_t row = 0; row < 7; ++row) {
        for (uint8_t col = 0; col < 5; ++col) {
            uint8_t line = glyph[col];
            uint8_t* p = &buf[(row * 5 + col) * 2];
            
            // Eğer o bit '1' ise rengi uygula, değilse arkaplan rengini uygula
            if (line & (1 << row)) {
                p[0] = hi; p[1] = lo;
            } else {
                p[0] = bgHi; p[1] = bgLo;
            }
        }
    }

    // Oluşan 5x7'lik kutuyu tek seferde (block write) ekrana gönder
    setAddressWindow(x, y, x + 4, y + 6);
    pinDC(true);
    pinCS(false);
    HAL_SPI_Transmit(hspi_, buf, sizeof(buf), HAL_MAX_DELAY);
    pinCS(true);
}

void St7735Display::writeCommand(uint8_t cmd)
{
    pinDC(false); // DC = LOW -> Komut gönderiliyor
    pinCS(false);
    HAL_SPI_Transmit(hspi_, &cmd, 1, HAL_MAX_DELAY);
    pinCS(true);
}

void St7735Display::writeData(const uint8_t* data, uint16_t len)
{
    pinDC(true); // DC = HIGH -> Veri gönderiliyor
    pinCS(false);
    HAL_SPI_Transmit(hspi_, const_cast<uint8_t*>(data), len, HAL_MAX_DELAY);
    pinCS(true);
}

void St7735Display::writeData8(uint8_t data)
{
    writeData(&data, 1);
}

void St7735Display::writeData16(uint16_t data)
{
    // ST7735 Big-Endian çalışır (Önce yüksek byte)
    uint8_t buf[2] = { static_cast<uint8_t>(data >> 8), static_cast<uint8_t>(data & 0xFF) };
    writeData(buf, 2);
}

void St7735Display::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // Çizim yapılacak dikdörtgen alanı belirler (CASET ve RASET komutları)
    writeCommand(ST7735_Cmd::ColAddrSet);
    writeData16(x0);
    writeData16(x1);

    writeCommand(ST7735_Cmd::RowAddrSet);
    writeData16(y0);
    writeData16(y1);

    // Belleğe yazmayı başlat (Bu komuttan sonra sadece renk verisi gönderilir)
    writeCommand(ST7735_Cmd::MemWrite);
}

void St7735Display::pinDC(bool level)
{
    HAL_GPIO_WritePin(DC_PORT, DC_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void St7735Display::pinCS(bool level)
{
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void St7735Display::pinRST(bool level)
{
    HAL_GPIO_WritePin(RST_PORT, RST_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
