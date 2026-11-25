# Ujian hidup Kawalan Majlis Studio

Ujian ini membuka Studio sebenar, memandunya melalui obs-websocket dan CDP (tanpa tetikus), kemudian
menutupnya seperti pengguna (WM_CLOSE). Perlu Node.js 22+ dan Windows.

Lalai: Studio yang dibina di `build_x64/rundir/RelWithDebInfo`. Tukar dengan `KM_RUNDIR`.
Gambar skrin disimpan di `%TEMP%\km-ujian` (tukar dengan `KM_UJI_OUT`).

| Fail | Apa yang diuji | Perlu |
|---|---|---|
| `uji-bilik.cjs` | Bilik Kawalan: CUT/FADE/T-bar/FTB, input, kekunci, REC/LIVE, tutup tanpa crash | scene ujian |
| `uji-ndi.cjs` | NDI: output Studio diterima semula sebagai input NDI | NDI Runtime + DistroAV |
| `uji-canva-panel.cjs` | NDI dari registri, Slaid Canva dari pautan pendek, ◀ ▶, PageDown, Skrin Projektor | `KM_CANVA_PENDEK` |
| `uji-canva.cjs` | Probe slaid Canva dalam browser source | pautan Canva / `KM_CANVA_URL` |
| `uji-kulit-album.cjs` | MP3 tidak memapar kulit album, bunyi lagu dan video masuk rakaman | `KM_UJI_LAGU`, `KM_UJI_VIDEO`, `KM_UJI_SCENE` |

`tangkap.ps1` mengambil gambar tetingkap Studio dengan PrintWindow (tidak mengganggu skrin pengguna).

Ujian panel web tanpa OBS: dalam submodul `kawalan-majlis/ujian/`.
