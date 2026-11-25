// Kawalan Majlis Studio: NDI check (DistroAV + NDI Runtime) without a camera:
// the Studio's own NDI output is received back as an NDI input, which must then show real video.
const path = require("path"), fs = require("fs");
const { spawn, execSync } = require("child_process");
const { obsSocket, sleep, until, ok, hasil, RD, BIN, CFG, tutup } = require("./sama.cjs");
// The NDI installer sets this machine-wide; a session started before the install does not have it yet.
function ndiDir() {
  try { return execSync(`powershell -NoProfile -Command "[Environment]::GetEnvironmentVariable('NDI_RUNTIME_DIR_V6','Machine')"`).toString().trim(); } catch (_) { return ""; }
}
(async () => {
  try { fs.unlinkSync(RD + "/config/obs-studio/.sentinel"); } catch (_) {}
  const app = spawn(path.join(BIN, "obs64.exe"), ["--portable", "--disable-shutdown-check", "--disable-updater"], { cwd: BIN, env: Object.assign({}, process.env, { KM_UJI: "1", NDI_RUNTIME_DIR_V6: ndiDir() }), detached: true, stdio: "ignore" });
  let o = null;
  for (let i = 0; i < 40 && !o; i++) { await sleep(500); try { const c = JSON.parse(fs.readFileSync(CFG, "utf8")); o = await obsSocket(c.server_password, c.server_port); } catch (_) {} }
  ok(!!o, "Studio bermula");
  if (!o) process.exit(1);
  await sleep(3000);
  const { inputKinds } = await o.call("GetInputKindList", { unversioned: true });
  ok(inputKinds.includes("ndi_source"), "DistroAV dimuat: jenis sumber NDI tersedia", inputKinds.filter(k => /ndi/i.test(k)).join(", "));

  // Our own NDI output on, then find it as an NDI source.
  const outs = (await o.call("GetOutputList")).outputs.map(x => x.outputName);
  const ndiOut = outs.find(n => /NDI Main Output/i.test(n));
  ok(!!ndiOut, "output NDI DistroAV wujud", outs.join(", "));
  // Program = moving test video, so the NDI loopback carries real motion (not an empty black scene).
  await o.call("SetStudioModeEnabled", { studioModeEnabled: false });
  await o.call("SetCurrentProgramScene", { sceneName: "Kamera Dewan" }).catch(() => {});
  if (ndiOut) await o.call("StartOutput", { outputName: ndiOut }).catch(e => console.log("StartOutput:", e.message));
  await o.call("CreateScene", { sceneName: "Kamera NDI Ujian" }).catch(() => {});
  await o.call("CreateInput", { sceneName: "Kamera NDI Ujian", inputName: "NDI Ujian", inputKind: "ndi_source", inputSettings: {}, sceneItemEnabled: true }).catch(() => {});
  const senarai = await until(async () => {
    try { const r = await o.call("GetInputPropertiesListPropertyItems", { inputName: "NDI Ujian", propertyName: "ndi_source_name" }); const a = r.propertyItems.filter(x => x.itemValue); return a.length ? a : null; } catch (_) { return null; }
  }, 15000);
  ok(!!senarai, "sumber NDI ditemui di rangkaian", senarai && senarai.map(x => x.itemName).join(" | "));
  if (senarai) {
    await o.call("SetInputSettings", { inputName: "NDI Ujian", inputSettings: { ndi_source_name: senarai[0].itemValue } });
    // Real video arrives: the screenshot of the NDI input is not a flat colour.
    const pelbagai = await until(async () => {
      const { imageData } = await o.call("GetSourceScreenshot", { sourceName: "NDI Ujian", imageFormat: "png", imageWidth: 64, imageHeight: 36 });
      const b = Buffer.from(imageData.split(",")[1], "base64");
      return b.length > 600 ? b.length : null;   // a flat image compresses to a few hundred bytes
    }, 15000);
    ok(!!pelbagai, "video NDI diterima (gambar bukan kosong)", pelbagai ? pelbagai + " bait" : "");
    const k = (await o.call("CallVendorRequest", { vendorName: "kawalan-majlis-studio", requestType: "keadaan", requestData: {} })).responseData;
    ok(k.jubin.some(j => j.nama === "Kamera NDI Ujian"), "kamera NDI muncul dalam deretan input Bilik Kawalan");
  }
  const s = await o.call("GetStats");
  ok(s.activeFps > 29, "OBS lancar dengan NDI", `fps ${s.activeFps.toFixed(1)}, CPU ${s.cpuUsage.toFixed(1)}%`);
  if (ndiOut) await o.call("StopOutput", { outputName: ndiOut }).catch(() => {});
  await o.call("RemoveScene", { sceneName: "Kamera NDI Ujian" }).catch(() => {});
  o.close();
  tutup(app.pid);   // the X on the window, like a person
  let keluar = false;
  for (let i = 0; i < 40 && !keluar; i++) { await sleep(500); try { process.kill(app.pid, 0); } catch (_) { keluar = true; } }
  ok(keluar, "Studio ditutup dengan bersih");
  if (!keluar) execSync(`taskkill /F /PID ${app.pid}`, { stdio: "ignore" });
  const { pass, fail } = hasil();
  console.log(`\n${pass} lulus, ${fail} gagal`);
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error(e); process.exit(2); });
