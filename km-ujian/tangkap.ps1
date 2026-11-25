# Capture the Kawalan Majlis Studio window with PrintWindow (never CopyFromScreen: the teacher uses this PC).
param([string]$Out = "$env:TEMP\km-tangkap.png", [int]$Tunggu = 14, [int]$Lebar = 0, [int]$Tinggi = 0, [string]$Akar = "$PSScriptRoot/../build_x64/rundir/RelWithDebInfo")
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices; using System.Text;
public class Tk {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr h, int x, int y, int w, int hh, bool r);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EP cb, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetWindowThreadProcessId(IntPtr h, out int pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  public delegate bool EP(IntPtr h, IntPtr p);
  public struct R { public int L, T, Ri, B; }
  public static IntPtr Cari(int pid, string tajuk) { IntPtr hasil = IntPtr.Zero; EnumWindows((h, p) => { int q; GetWindowThreadProcessId(h, out q); var sb = new StringBuilder(256); GetWindowText(h, sb, 256); if (q == pid && IsWindowVisible(h) && sb.ToString() == tajuk) { hasil = h; return false; } return true; }, IntPtr.Zero); return hasil; }
}
"@
$bin = "$Akar/bin/64bit"
Remove-Item "$Akar/config/obs-studio/.sentinel/run_*" -ErrorAction SilentlyContinue
$p = Start-Process -FilePath "$bin\obs64.exe" -WorkingDirectory $bin -ArgumentList '--portable','--disable-shutdown-check','--disable-updater' -PassThru
Start-Sleep $Tunggu
$h = [Tk]::Cari($p.Id, "Kawalan Majlis Studio")
if ($h -eq [IntPtr]::Zero) { "tetingkap tidak dijumpai" } else {
  if ($Lebar -gt 0) { [void][Tk]::ShowWindow($h, 9); [void][Tk]::MoveWindow($h, 0, 0, $Lebar, $Tinggi, $true); Start-Sleep 2 }
  $r = New-Object Tk+R; [void][Tk]::GetWindowRect($h, [ref]$r)
  $bmp = New-Object Drawing.Bitmap ($r.Ri - $r.L), ($r.B - $r.T); $g = [Drawing.Graphics]::FromImage($bmp); $dc = $g.GetHdc()
  [void][Tk]::PrintWindow($h, $dc, 2); $g.ReleaseHdc($dc); $bmp.Save($Out); $g.Dispose(); $bmp.Dispose()
  "disimpan $Out ($($r.Ri - $r.L) x $($r.B - $r.T))"
}
taskkill /PID $p.Id | Out-Null
if (-not $p.WaitForExit(15000)) { Stop-Process -Id $p.Id -Force; "terpaksa tutup" }
