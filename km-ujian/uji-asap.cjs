// Smoke test of a Studio folder as a teacher gets it (e.g. the release zip, extracted):
//   KM_RUNDIR="C:\path\Kawalan Majlis Studio" node km-ujian/uji-asap.cjs
// First start: obs-websocket set up by itself, Bilik Kawalan open (main OBS window hidden), panel loaded and
// connected, no setup wizard, NDI source type available, closes without a crash report.
const path = require("path"), fs = require("fs");
const { spawn } = require("child_process");
const { obsSocket, sleep, until, tab, ok, hasil, BIN, CFG, CRASH, tutup } = require("./sama.cjs");

function tetingkap(pid) {
  const ps = `Add-Type @"
using System; using System.Runtime.InteropServices; using System.Text; using System.Collections.Generic;
public class W { [DllImport("user32.dll")] public static extern bool EnumWindows(P cb, IntPtr p);
 [DllImport("user32.dll")] public static extern int GetWindowThreadProcessId(IntPtr h, out int pid);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
 public delegate bool P(IntPtr h, IntPtr p);
 public static string Senarai(int pid) { var l = new List<string>(); EnumWindows((h, p) => { int q; GetWindowThreadProcessId(h, out q); if (q == pid && IsWindowVisible(h)) { var sb = new StringBuilder(256); GetWindowText(h, sb, 256); if (sb.Length > 0) l.Add(sb.ToString()); } return true; }, IntPtr.Zero); return string.Join("|", l); } }
"@
[W]::Senarai(${pid})`;
  return require("child_process").execFileSync("powershell", ["-NoProfile", "-Command", ps]).toString().trim();
}

(async () => {
  const mula = Date.now();
  ok(!fs.existsSync(CFG), "folder bersih: tiada tetapan websocket lagi (permulaan pertama)");
  const app = spawn(path.join(BIN, "obs64.exe"), ["--remote-debugging-port=9223", "--disable-updater", "--disable-shutdown-check"], { cwd: BIN, detached: true, stdio: "ignore" });
  let o = null;
  for (let i = 0; i < 60 && !o; i++) { await sleep(500); try { const c = JSON.parse(fs.readFileSync(CFG, "utf8")); o = await obsSocket(c.server_password, c.server_port); } catch (_) {} }
  ok(!!o, "obs-websocket dihidupkan sendiri dengan kata laluan");
  if (!o) { tutup(app.pid); process.exit(1); }
  const pw = JSON.parse(fs.readFileSync(CFG, "utf8")).server_password;
  ok(pw.length >= 16, "kata laluan websocket rawak (16 aksara)");
  // First start in a fresh folder is slow (Chromium builds its caches): wait for the window.
  const tt = (await until(() => { const t = tetingkap(app.pid); return t.split("|").includes("Kawalan Majlis Studio") ? t : null; }, 45000)) || tetingkap(app.pid);
  ok(tt.split("|").includes("Kawalan Majlis Studio"), "Bilik Kawalan terbuka", tt);
  ok(!/OBS \d|Auto-Configuration|Wizard/i.test(tt), "tiada tetingkap OBS utama atau wizard", tt);

  const pg = await until(() => tab(/kawalan-majlis\.html/), 45000);
  ok(!!pg, "panel Kawalan Majlis dimuat");
  if (pg) {
    // A fresh profile has no name overlay yet: the panel is connected and asks for Persediaan (yellow "wait").
    const sambung = await until(() => pg.evaluate(() => { const c = document.getElementById("kmConn"); return /(ok|wait)/.test(c.className) ? c.className + " · " + document.getElementById("kmConnMsg").textContent : null; }), 15000);
    ok(!!sambung, "panel bersambung ke Studio", sambung || await pg.evaluate(() => document.getElementById("kmConn").className + " · " + document.getElementById("kmConnMsg").textContent + " · " + location.search.replace(/pw=[^&]*/, "pw=***")));
    ok(await pg.evaluate(() => /GPL/.test(document.body.textContent)), "panel memapar lesen GPL");
    ok(pg.errs.length === 0, "tiada ralat dalam panel", pg.errs.slice(0, 2).join(" | "));
    pg.close();
  }
  const { inputKinds } = await o.call("GetInputKindList", { unversioned: true });
  ok(inputKinds.includes("ndi_source"), "sumber NDI (DistroAV) tersedia");
  ok((await o.call("GetStudioModeEnabled")).studioModeEnabled, "Studio Mode hidup");
  const prog = await o.call("GetCurrentProgramScene").catch(e => ({ ralat: e.message }));
  ok(!!prog.currentProgramSceneName, "scene Program wujud (Studio Mode pada profil baharu)", prog.currentProgramSceneName || prog.ralat);
  const khas = await o.call("GetSpecialInputs");
  ok(!!(khas.desktop1 || khas.mic1), "peranti audio dicipta pada permulaan pertama", JSON.stringify(khas));
  o.close();

  tutup(app.pid);
  let keluar = false;
  for (let i = 0; i < 40 && !keluar; i++) { await sleep(500); try { process.kill(app.pid, 0); } catch (_) { keluar = true; } }
  ok(keluar, "Studio ditutup dengan butang X");
  const crash = fs.existsSync(CRASH) ? fs.readdirSync(CRASH).filter(f => fs.statSync(path.join(CRASH, f)).mtimeMs > mula) : [];
  ok(crash.length === 0, "tiada laporan crash");
  const { pass, fail } = hasil();
  console.log(`\n${pass} lulus, ${fail} gagal`);
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error(e); process.exit(2); });
