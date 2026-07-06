# YD-RP2040 + OV7670 — Capture Test (Pico SDK, C)

Project minimal untuk verifikasi bahwa OV7670 bisa di-capture oleh YD-RP2040
lewat PIO+DMA. Frame di-capture lalu dicetak sebagai ASCII art ke serial USB
— cukup untuk memastikan wiring & kamera bekerja sebelum lanjut ke tahap
berikutnya (simpan gambar, streaming ke PC, tampil di layar, dst).

Bukan pakai firmware OpenMV — OpenMV butuh hardware DCMI + RAM besar yang
RP2040 tidak punya. Ini pakai library PIO-based murni C:
[usedbytes/camera-pico-ov7670](https://github.com/usedbytes/camera-pico-ov7670)
(di-fetch otomatis oleh CMake, tidak perlu clone manual).

## Komponen (BOM)

- 1x YD-RP2040 (atau board RP2040 lain yang pin-compatible dengan Pico)
- 1x kamera OV7670 (modul dengan breakout, ada pin SIOC/SIOD/VSYNC/HREF/PCLK/XCLK/D0-D7)
- 1x IC 74HC165 (shift register paralel-ke-serial, DIP-16/SOIC-16)
- Kabel jumper + breadboard

74HC165 dipakai supaya cuma butuh 2 pin RP2040 untuk 8 bit data paralel
kamera (hemat pin, dan ini yang sudah teruji jalan di library-nya). Konversi
ke wiring 8-bit paralel langsung juga mungkin tapi butuh PIO custom yang
belum teruji — untuk verifikasi awal, jalur 74HC165 ini lebih aman dipakai.

## Wiring

> Ganti pin di bawah kalau perlu — semuanya bisa diubah lewat `CAMERA_BASE_PIN`,
> `CAMERA_XCLK_PIN`, `CAMERA_SDA`, `CAMERA_SCL` di [src/main.c](src/main.c).
> Pin default sengaja dijauhkan dari GP16/GP25 karena beberapa varian
> YD-RP2040 memakai pin itu untuk LED/WS2812 onboard — cek dulu board kamu.

### OV7670 <-> YD-RP2040

| OV7670 | YD-RP2040 |
|--------|-----------|
| 3.3V   | 3V3 (OUT) |
| GND    | GND |
| SIOC (SCL) | GP5 |
| SIOD (SDA) | GP4 |
| VSYNC  | GP8 |
| HREF   | GP9 |
| PCLK   | GP10 (juga ke 74HC165 pin 1, lihat tabel bawah) |
| XCLK   | GP21 |
| RESET  | 3.3V (tie high) |
| PWDN   | GND |
| D0     | 74HC165 pin 11 (H) |
| D1     | 74HC165 pin 12 (G) |
| D2     | 74HC165 pin 13 (F) |
| D3     | 74HC165 pin 14 (E) |
| D4     | 74HC165 pin 3 (D) |
| D5     | 74HC165 pin 4 (C) |
| D6     | 74HC165 pin 5 (B) |
| D7     | 74HC165 pin 6 (A) |

### 74HC165 <-> YD-RP2040

| 74HC165 pin | Nama | Sambung ke |
|---|---|---|
| 1  | SH/nLD   | OV7670 PCLK **dan** GP10 (paralel, satu net) |
| 2  | CLK      | GP7 |
| 9  | QH (serial out) | GP6 |
| 15 | CLK INH  | GND |
| 16 | VCC      | 3.3V |
| 8  | GND      | GND |
| 7  | QH̄ (QH complement) | tidak dipakai |
| 10 | SER (serial in) | tidak dipakai |

Data D0-D7 dari OV7670 masuk ke input paralel 74HC165 (pin A-H) seperti
tabel di atas, lalu dibaca RP2040 secara serial lewat pin QH.

## Setup Toolchain (Windows)

Toolchain-nya sudah diinstall & di-build sekali di komputer ini (lewat
`winget`), jadi tidak perlu setup ulang:

- `Arm.ArmGnuToolchain` (arm-none-eabi-gcc, untuk compile target RP2040)
- `BrechtSanders.WinLibs.POSIX.UCRT` (gcc/g++ native, dibutuhkan CMake untuk
  build tool internal Pico SDK yaitu `pioasm` dan `picotool`)

Keduanya sudah otomatis masuk ke PATH user secara permanen. Kalau kamu pindah
ke komputer lain, cara paling gampang adalah install ekstensi **"Raspberry Pi
Pico"** di VS Code — otomatis men-download Pico SDK + toolchain + CMake +
Ninja, dan menyediakan tombol *Compile*/*Run* untuk project CMake seperti ini.

CMake project ini pakai `PICO_SDK_FETCH_FROM_GIT=1` supaya Pico SDK
otomatis ter-download saat konfigurasi pertama kali (tidak perlu clone
manual) — didownload ke `build/_deps/pico_sdk-src`.

## Build

**Penting:** buka terminal **baru** dulu (supaya PATH yang diupdate installer
terbaca), lalu:

```powershell
cd g:\project_openmv
mkdir build
cd build
$env:PICO_SDK_FETCH_FROM_GIT = "1"
cmake .. -G Ninja
ninja
```

Sudah diverifikasi build sukses sampai selesai di project ini — hasilnya ada
di `build/ov7670_test.uf2` (~84KB).

## Flash

1. Tekan & tahan tombol **BOOTSEL** di YD-RP2040, colok USB, lepas BOOTSEL.
   Board akan muncul sebagai mass-storage drive (`RPI-RP2`).
2. Copy `build/ov7670_test.uf2` ke drive tersebut. Board otomatis reboot dan
   langsung menjalankan program.

## Lihat hasil capture

Buka serial monitor ke COM port board ini (baud rate tidak berpengaruh,
USB CDC) — misalnya lewat PuTTY, `Tera Term`, atau `python -m serial.tools.miniterm COMx 115200`.
Setiap ~3 detik akan muncul log `Capturing...` diikuti ASCII art hasil
capture 80x60.

## Troubleshooting

- **`camera_init failed: -1`** — OV7670 tidak terdeteksi di I2C. Cek wiring
  SIOC/SIOD, pastikan XCLK jalan (kamera butuh clock dulu sebelum bisa
  dibaca lewat I2C), cek PWDN ditarik GND dan RESET ditarik 3.3V.
- **LED berkedip cepat terus** — sama seperti di atas, itu sinyal error dari
  `main.c` saat `camera_init` gagal.
- **Gambar ASCII kosong/acak** — biasanya wiring 74HC165 atau PCLK/HREF/VSYNC
  yang salah. Cek ulang tabel wiring, terutama SH/nLD yang harus satu net
  dengan PCLK.
- Tidak ada output sama sekali di serial monitor — tunggu ~3 detik setelah
  power-up (ada `sleep_ms(3000)` supaya USB serial sempat konek), atau pastikan
  driver USB CDC RP2040 sudah terpasang di Windows.
