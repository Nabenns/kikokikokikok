# GTPS Community Server — Setup Client

Server IP: `103.253.213.178`
Game port: `17091` (UDP)

## Windows (paling gampang)

1. Buka **Notepad** sebagai Administrator (klik kanan → Run as administrator).
2. File → Open → browse ke: `C:\Windows\System32\drivers\etc\hosts`
3. Tambahin 2 baris ini di paling bawah:
   ```
   103.253.213.178 www.growtopia1.com
   103.253.213.178 www.growtopia2.com
   ```
4. Save (Ctrl+S).
5. Buka Growtopia seperti biasa.
6. Login/register akun baru (akun GTPS terpisah dari official).

## Android

### Opsi A: Virtual Hosts app
1. Download **Virtual Hosts** dari Play Store.
2. Buat file `hosts.txt` dengan isi:
   ```
   103.253.213.178 www.growtopia1.com
   103.253.213.178 www.growtopia2.com
   ```
3. Buka Virtual Hosts → Select Host File → pilih file tadi.
4. Buka Growtopia.

### Opsi B: Modded APK
Pakai launcher/custom APK yang langsung redirect ke IP server.

## iOS (iPhone/iPad)

1. Download **Surge 5** dari App Store.
2. Buka Surge 5 → tap **Default.conf**.
3. Pergi ke **Import → Download Profile from URL**, masukkan URL profile (atau manual):
   - Pergi ke **DNS → Local Mapping**
   - Add New:
     - Domain: `www.growtopia1.com` → Value: `103.253.213.178`
     - Domain: `www.growtopia2.com` → Value: `103.253.213.178`
4. Tap **Setup** → allow VPN configuration.
5. Buka Growtopia.

## macOS

1. Finder → Go → Go to Folder → `/private/etc/hosts`
2. Drag file `hosts` ke Desktop.
3. Buka dengan TextEdit, tambah:
   ```
   103.253.213.178 www.growtopia1.com
   103.253.213.178 www.growtopia2.com
   ```
4. Save, drag balik ke `/private/etc/`, replace.
5. Buka Growtopia.

## Cara cek berhasil
- Buka Growtopia → harusnya masuk ke server private (bukan official).
- Kalau stuck "Connecting..." atau error, cek:
  - File hosts sudah benar (no typo)
  - Run as admin waktu edit (Windows)
  - VPN/proxy lain dimatikan dulu

## Kembali ke official server
- Windows: hapus 2 baris tadi dari file hosts, save.
- Android: stop Virtual Hosts app.
- iOS: disable Surge 5.
- macOS: hapus baris dari hosts.

## Troubleshooting connect failure
| Gejala | Kemungkinan |
|---|---|
| Stuck "Connecting..." | hosts file salah / belum di-save sebagai admin |
| "Connection failed" setelah login | UDP 17091 ke-block (firewall client) |
| "Maintenance" padahal server up | config maintenance=false, clear cache client |
| Login page ga muncul | coba akses `https://103.253.213.178` di browser |

## Info server
- Engine: GrowServer v3.2.0 (Node.js + ENet)
- items.dat: v5.47 (16,150 items)
- DB: PostgreSQL 17
- Cache: Redis 7
- Commands: 38 (try `/help` in-game)
