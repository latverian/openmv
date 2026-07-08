# OpenMV Firmware for YD-RP2040 (RPI_PICO target) — OV7670

Ini **firmware OpenMV asli** (bukan project C custom kita sebelumnya),
di-build dari source resmi [openmv/openmv](https://github.com/openmv/openmv)
untuk target board `RPI_PICO`, supaya YD-RP2040 bisa dipakai dengan
**OpenMV IDE** sungguhan sesuai requirement dosen.

## Status jujur

Board target `RPI_PICO` ada di source OpenMV tapi **tidak pernah dirilis
resmi** dan sempat rusak total di source (salah satu maintainer OpenMV
bilang langsung "*the Pico just doesn't build anymore ... it's a very
useless MCU anyway*"). Untuk membuatnya bisa di-build sampai jadi
`firmware.uf2`, saya menemukan dan menambal **~15 bug nyata** di source
resminya (bukan modifikasi custom kita — semuanya bug/kekurangan di
upstream `openmv/openmv` sendiri untuk port RP2040 ini), di antaranya:

- Script SDK installer salah (CRLF, flag `tar` yang hilang)
- Variable `OMV_CMAKE` tidak ke-pass ke build submodule
- Kode `usbdbg.c`/`tinyusb_debug.c` yang sudah dihapus upstream tapi
  masih direferensikan di CMake RP2 → dipindah ke sistem protokol baru
  (`protocol/omv_protocol*.c`)
- `fb_alloc.c`/`dma_alloc.c` yang sudah diganti sistem allocator baru
  (UMA) tapi RP2 belum diupdate
- Bug environment variable (`MAKE=@make`) yang bikin sub-build gagal
- Beberapa path build directory yang salah (hardcoded, tidak match target board)
- `omv_cycles.h` pakai register DWT yang cuma ada di Cortex-M3/M4/M7, RP2040
  itu Cortex-M0+ (tidak punya DWT) → diganti pakai timer hardware Pico SDK
- Fungsi I2C `_sccb`/`_reg` yang dibutuhkan driver kamera & sensor lain
  belum diimplementasi di port RP2
- Linker script kekurangan section untuk UMA memory pool & USB protocol channel

Detail lengkap tiap perubahan ada di [rp2-rpi_pico-fixes.patch](rp2-rpi_pico-fixes.patch)
(format `git diff`, bisa dibaca langsung atau di-apply ke clone `openmv/openmv`).

**Artinya:** firmware ini sudah **berhasil di-build sampai selesai** dan
secara teori seharusnya bisa capture gambar dari OV7670 + connect ke OpenMV
IDE — tapi karena ini jalur yang belum pernah dites orang lain sampai
berhasil, **belum ada verifikasi di hardware asli**. Langkah berikutnya
adalah kamu coba di board fisik dan kita debug bareng kalau ada masalah.

## File di folder ini

- `firmware.uf2` — **ini yang di-flash ke YD-RP2040**
- `firmware.elf` — versi dengan debug symbols (buat debugging kalau perlu)
- `rp2-rpi_pico-fixes.patch` — semua perubahan source yang saya buat, untuk transparansi

## Wiring OV7670 → YD-RP2040

**Beda dari project C sebelumnya** — firmware OpenMV resmi ini pakai wiring
**paralel 8-bit langsung**, TIDAK butuh IC 74HC165 lagi:

| OV7670 | YD-RP2040 (GPIO) |
|--------|-------------------|
| 3.3V   | 3V3 (OUT) |
| GND    | GND |
| SIOC (SCL) | GP15 |
| SIOD (SDA) | GP14 |
| VSYNC  | GP13 |
| HREF/HSYNC | GP12 |
| PCLK   | GP11 |
| XCLK   | GP10 |
| PWDN   | GP0 |
| RESET  | GP1 |
| D0     | GP2 |
| D1     | GP3 |
| D2     | GP4 |
| D3     | GP5 |
| D4     | GP6 |
| D5     | GP7 |
| D6     | GP8 |
| D7     | GP9 |

Sumber: [`boards/RPI_PICO/board_config.h`](https://github.com/openmv/openmv/blob/master/boards/RPI_PICO/board_config.h)
di source OpenMV (pin ini sudah didefinisikan resmi, saya tidak mengubahnya).

> LED onboard board pakai GP25 (standar Pico) — kalau varian YD-RP2040 kamu
> pakai GP25 untuk hal lain, cek dulu sebelum flashing.

## Cara flash

1. Tekan & tahan tombol **BOOTSEL** di YD-RP2040 sambil colok USB, lepas BOOTSEL.
2. Board muncul sebagai drive `RPI-RP2`.
3. Copy `firmware.uf2` ke drive itu. Board reboot otomatis.

## Cara connect ke OpenMV IDE

1. Install [OpenMV IDE](https://openmv.io/pages/download).
2. Colok YD-RP2040 (yang sudah di-flash firmware ini) ke USB.
3. Buka OpenMV IDE, klik connect.

**Kemungkinan kendala:** OpenMV IDE punya daftar board yang dikenali lewat
USB VID:PID. Firmware ini pakai VID `0x2E8A` / PID `0x0005` (sudah didefinisikan
resmi di board_config RPI_PICO), tapi IDE versi kamu mungkin belum
mengenali kombinasi ini di daftar board bawaan. Kalau IDE tidak mau connect:
- Coba update ke OpenMV IDE versi terbaru dulu
- Kalau masih gagal, ada workaround edit `settings.json` IDE untuk menambahkan
  board ini secara manual — kabari saya kalau sampai di titik ini, saya bantu.

## Kalau ada masalah pas dicoba di hardware

Kabari gejalanya (LED nyala/mati, ada output di serial monitor atau tidak,
IDE bisa connect atau tidak, dst) — kemungkinan besar akan ada bug lanjutan
yang perlu ditambal, sama seperti proses build-nya tadi.
