# Close the Kawalan Majlis Studio the way a person does: the X on the "Kawalan Majlis Studio" window (WM_CLOSE).
param([int]$ProsesId)
Add-Type @"
using System; using System.Runtime.InteropServices; using System.Text;
public class Tt {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EP cb, IntPtr p);
  [DllImport("user32.dll")] public static extern int GetWindowThreadProcessId(IntPtr h, out int pid);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  public delegate bool EP(IntPtr h, IntPtr p);
  public static bool Tutup(int pid) { bool jumpa = false; EnumWindows((h, p) => { int q; GetWindowThreadProcessId(h, out q); var sb = new StringBuilder(256); GetWindowText(h, sb, 256); if (q == pid && sb.ToString() == "Kawalan Majlis Studio") { PostMessage(h, 0x0010, IntPtr.Zero, IntPtr.Zero); jumpa = true; } return true; }, IntPtr.Zero); return jumpa; }
}
"@
if ([Tt]::Tutup($ProsesId)) { "WM_CLOSE dihantar" } else { "tetingkap tidak dijumpai" }
