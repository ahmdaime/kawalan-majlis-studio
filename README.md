# Kawalan Majlis Studio

Aplikasi siaran langsung dan rakaman untuk **majlis sekolah**, dibina atas [OBS Studio](https://obsproject.com)
(GPL-2.0). Dibina oleh seorang guru di Malaysia supaya guru lain boleh mengendalikan siaran majlis seperti
studio TV, tanpa perlu mahir OBS.

> Kawalan Majlis Studio ialah fork bebas OBS Studio. Ia **bukan** produk rasmi OBS Project dan tidak disokong
> oleh mereka. Masalah dengan aplikasi ini: laporkan di sini, bukan kepada OBS Project.

## Apa yang ada

- **Bilik Kawalan** terus sebaik dibuka: PREVIEW (oren) dan PROGRAM (merah), CUT / FADE / T-bar / FTB,
  deretan input (kekunci 1–9, Enter = CUT, F = FADE), audio, REC, LIVE (henti = tahan 2 saat).
- **Panel Pengarah** ([kawalan-majlis](https://github.com/ahmdaime/kawalan-majlis)) di sebelah kanan:
  - senarai penerima dari Excel (Nama, Kelas, Anugerah), butang kuning **PAPAR** satu-satunya cara ke udara;
  - aturcara berturutan: nama, lagu, video, slaid PDF, gambar; muzik latar yang mengecil sendiri;
  - **slaid Canva** dengan animasi (tampal pautan; tukar slaid dengan ◀ ▶ atau clicker PageUp/PageDown);
  - input **Skrin Projektor** (tangkap skrin kedua, contohnya Canva Present);
  - pereka paparan nama, termasuk templat **Kad hijau emas**.
- **Kamera NDI** (contohnya kamera PTZ) melalui [DistroAV](https://github.com/DistroAV/DistroAV).
  Jika NDI belum dipasang, Studio memberitahu dalam Bahasa Melayu dan membuka laman muat turun rasmi.
- Pilihan lalai yang selamat untuk majlis: Studio Mode tanpa "Duplicate Scene" (lagu dan video sentiasa
  sampai ke siaran), lagu MP3 hanya berbunyi (gambar kulit album tidak dipapar).
- Mudah alih: satu folder, tidak perlu dipasang. Tetapan disimpan dalam folder itu sendiri.

Semua tetapan OBS penuh masih ada: butang ⚙ (Tetapan lanjutan).

## Muat turun dan cuba (Windows 10/11, 64-bit)

1. Muat turun zip terkini di **[Releases](https://github.com/ahmdaime/kawalan-majlis-studio/releases)**.
2. Ekstrak ke folder, contohnya `C:\Kawalan Majlis Studio`.
3. Klik dua kali `bin\64bit\obs64.exe`.
4. Untuk kamera NDI: pasang [NDI Tools](https://ndi.video/tools/download/) (percuma).

Panduan penggunaan panel: `data\obs-studio\kawalan-majlis\PASANG.html` dalam folder zip.

## Mahu membantu?

- **Uji** di sekolah anda dan laporkan masalah atau cadangan di tab **Issues** (sertakan gambar skrin dan
  langkah untuk mengulang masalah).
- **Pembaikan kod:** hantar **Pull Request**. Kod Bilik Kawalan: `frontend/widgets/BilikKawalan.*`.
  Panel web ialah submodul `kawalan-majlis/` (repo berasingan).

## Membina dari kod sumber

Sama seperti OBS Studio 32 di Windows ([panduan OBS](https://github.com/obsproject/obs-studio/wiki/Build-Instructions-For-Windows)):
Visual Studio 2022, CMake 3.28+, Windows SDK 10.0.22621+. Bina di pemacu NTFS (bukan exFAT).

```
git clone --recursive https://github.com/ahmdaime/kawalan-majlis-studio.git
cd kawalan-majlis-studio
cmake --preset windows-x64
cmake --build build_x64 --config RelWithDebInfo
```

Hasil: `build_x64\rundir\RelWithDebInfo\bin\64bit\obs64.exe`. Untuk kamera NDI, salin DistroAV (versi
mudah alih) ke `obs-plugins\64bit` dan `data\obs-plugins`.

Ujian hidup (tanpa tetikus, melalui obs-websocket dan CDP): lihat `km-ujian/README.md`.

## Lesen

GNU General Public License versi 2 atau kemudian (GPL-2.0-or-later), sama seperti OBS Studio. Lihat `COPYING`.
Hak cipta OBS Studio milik penyumbang OBS Project; tambahan Kawalan Majlis (c) 2026 Cikgu Aime dan penyumbang.
Nama dan logo "OBS" ialah tanda dagangan OBS Project dan tidak digunakan sebagai nama produk ini.

README asal OBS Studio: `README.rst`.
