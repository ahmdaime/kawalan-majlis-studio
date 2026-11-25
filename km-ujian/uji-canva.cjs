// Probe / test: the Canva embed inside the Studio's own browser source.
// Usage: node uji-canva.cjs [url] [mod]   mod = "dom" (inspect the control bar) | "kunci" (arrow keys via CDP) | "vendor" (keys via the Studio's own request)
const path = require("path"), fs = require("fs");
const { spawn, execSync } = require("child_process");
const { obsSocket, sleep, until, tab, BIN, CFG, OUT, tutup } = require("./sama.cjs");
const URL = process.argv[2] || process.env.KM_CANVA_URL;
if (!URL) { console.log("LANGKAU: beri pautan Canva (argumen pertama atau KM_CANVA_URL)"); process.exit(0); }
const MOD = process.argv[3] || "dom";
const SRC = "Kawalan Majlis - Canva";
(async () => {
  const app = spawn(path.join(BIN, "obs64.exe"), ["--portable", "--remote-debugging-port=9223", "--disable-shutdown-check", "--disable-updater"], { cwd: BIN, detached: true, stdio: "ignore" });
  let o = null;
  for (let i = 0; i < 40 && !o; i++) { await sleep(500); try { const c = JSON.parse(fs.readFileSync(CFG, "utf8")); o = await obsSocket(c.server_password, c.server_port); } catch (_) {} }
  await sleep(2000);
  await o.call("CreateScene", { sceneName: "Uji Canva" }).catch(() => {});
  await o.call("CreateInput", { sceneName: "Uji Canva", inputName: SRC, inputKind: "browser_source", inputSettings: { url: URL, width: 1920, height: 1080, fps: 30, reroute_audio: true, css: process.env.CSS || "" }, sceneItemEnabled: true }).catch(e => console.log(e.message));
  const snap = async n => { const { imageData } = await o.call("GetSourceScreenshot", { sourceName: SRC, imageFormat: "png", imageWidth: 1280, imageHeight: 720 }); fs.writeFileSync(path.join(OUT, `canva-${n}.png`), Buffer.from(imageData.split(",")[1], "base64")); };
  await sleep(12000); await snap("0");
  const t = await until(() => tab(/canva\.com/), 10000);
  if (MOD === "dom") {
    console.log(await t.evaluate(() => {
      const out = [];
      for (const [x, y] of [[40, 1050], [100, 1050], [1880, 1050]]) {
        let e = document.elementFromPoint(x, y); const ch = [];
        while (e && e !== document.body) { const r = e.getBoundingClientRect(); ch.push(e.tagName + (e.getAttribute("aria-label") ? "[" + e.getAttribute("aria-label") + "]" : "") + "." + String(e.className).slice(0, 30) + " h" + Math.round(r.height) + " y" + Math.round(r.top)); e = e.parentElement; }
        out.push(ch.join(" < "));
      }
      return out.join(" || ") + " || vp " + innerWidth + "x" + innerHeight;
    }));
  } else {
    const ws = MOD === "vendor";
    const step = async () => {
      if (ws) return console.log("vendor:", JSON.stringify(await o.call("CallVendorRequest", { vendorName: "kawalan-majlis", requestType: "canva", requestData: { arah: 1 } })));
      for (const type of ["rawKeyDown", "keyUp"]) await t.send("Input.dispatchKeyEvent", { type, key: "ArrowRight", code: "ArrowRight", windowsVirtualKeyCode: 39, nativeVirtualKeyCode: 39 });
    };
    for (const n of [1, 2, 3, 4]) { await step(); await sleep(3000); await snap(n); console.log(n, await t.evaluate(() => (document.querySelector("footer") || {}).innerText)); }
    console.log("halaman:", await t.evaluate(() => (document.body.innerText.match(/\d+ \/ \d+/) || [""])[0]));
  }
  await o.call("RemoveScene", { sceneName: "Uji Canva" }).catch(() => {});
  o.close();
  tutup(app.pid);
  for (let i = 0; i < 30; i++) { await sleep(500); try { process.kill(app.pid, 0); } catch (_) { process.exit(0); } }
  console.log("Studio tidak tertutup");
  process.exit(1);
})();
