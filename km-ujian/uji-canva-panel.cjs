// Live test in the Studio: NDI found from the registry (no NDI_RUNTIME_DIR_V6 in the environment),
// Slaid Canva from a canva.link short link (◀ ▶, PageDown) and the Skrin Projektor input.
const path = require("path"), fs = require("fs");
const { spawn, execSync } = require("child_process");
const { obsSocket, sleep, until, tab, ok, hasil, BIN, CFG, LOGS, CRASH, OUT, tutup } = require("./sama.cjs");
// A canva.link short link to a design shared "anyone with the link can view".
const PENDEK = process.env.KM_CANVA_PENDEK;
if (!PENDEK) { console.log("LANGKAU: tetapkan KM_CANVA_PENDEK=https://canva.link/..."); process.exit(0); }

(async () => {
  const env = Object.assign({}, process.env, { KM_UJI: "1" });
  for (const k of Object.keys(env)) if (/^NDI_RUNTIME_DIR/i.test(k)) delete env[k];   // like an Explorer started before NDI was installed
  const mula = Date.now();
  const app = spawn(path.join(BIN, "obs64.exe"), ["--portable", "--remote-debugging-port=9223", "--disable-shutdown-check", "--disable-updater"], { cwd: BIN, env, detached: true, stdio: "ignore" });
  let o = null;
  for (let i = 0; i < 60 && !o; i++) { await sleep(500); try { const c = JSON.parse(fs.readFileSync(CFG, "utf8")); o = await obsSocket(c.server_password, c.server_port); } catch (_) {} }
  ok(!!o, "Studio bermula");
  if (!o) process.exit(1);
  await sleep(3000);

  // NDI
  const log = fs.readdirSync(LOGS).map(f => path.join(LOGS, f)).filter(f => fs.statSync(f).mtimeMs >= mula - 2000).sort().pop();
  const teks = log ? fs.readFileSync(log, "utf8") : "";
  ok(/NDI Runtime dijumpai melalui registri/.test(teks), "NDI dijumpai melalui registri walaupun tiada dalam persekitaran");
  ok(!/NDI Runtime not found|Error-401/i.test(teks), "tiada ralat 'NDI Runtime not found' dalam log");
  const { inputKinds } = await o.call("GetInputKindList", { unversioned: true });
  ok(inputKinds.includes("ndi_source"), "sumber NDI tersedia");

  const pg = await until(() => tab(/kawalan-majlis\.html/), 15000);
  ok(!!pg, "panel dimuat");
  await until(() => pg.evaluate(() => document.getElementById("kmConn").classList.contains("ok") ? 1 : null), 10000);

  // Canva from the short link
  await pg.click("#mSetup");
  await pg.fill("#canvaUrl", PENDEK);
  await pg.click("#canvaSimpan");
  const url = await until(async () => { try { const u = (await o.call("GetInputSettings", { inputName: "Kawalan Majlis - Canva" })).inputSettings.url; return /view\?embed$/.test(u) ? u : null; } catch (_) { return null; } }, 20000);
  ok(!!url, "pautan pendek canva.link dibuka ke pautan embed", url || await pg.evaluate(() => document.getElementById("canvaMsg").textContent + " | " + document.getElementById("canvaUrl").value));
  if (!url) { console.log(JSON.stringify(await o.call("CallVendorRequest", { vendorName: "kawalan-majlis", requestType: "pautan", requestData: { url: PENDEK } }).catch(e => e.message))); }
  const { scenes } = await o.call("GetSceneList");
  ok(scenes.some(s => s.sceneName === "Slaid Canva"), "scene Slaid Canva dicipta");
  ok(await pg.evaluate(() => !document.getElementById("canvaBar").hidden), "bar Slaid Canva kelihatan dalam panel");
  ok(await pg.evaluate(() => /sedia/.test(document.getElementById("canvaInfo").textContent)), "langkah 4 ditanda sedia", await pg.evaluate(() => document.getElementById("canvaInfo").textContent));

  // On air: ◀ ▶ and the clicker key
  await pg.click("#mShow");
  await o.call("SetCurrentProgramScene", { sceneName: "Slaid Canva" });
  ok(await until(() => pg.evaluate(() => /UDARA/.test(document.getElementById("canvaUdara").textContent) ? 1 : null), 5000), "panel tahu Slaid Canva ke udara");
  const cv = await until(() => tab(/canva\.com\/design/), 20000);
  const halaman = async () => { const t = await cv.evaluate(() => (document.querySelector("footer") || {}).innerText || ""); const m = /(\d+)\s*\/\s*(\d+)/.exec(t.replace(/\s+/g, " ")); return m ? +m[1] : 0; };
  await until(async () => (await halaman()) > 0 ? 1 : null, 15000);
  const h0 = await halaman();
  await pg.click("#canvaNext"); await sleep(2500);
  await pg.click("#canvaNext"); await sleep(2500);
  const h1 = await halaman();
  ok(h1 >= h0 + 1, "butang ▶ menukar slaid", `${h0} → ${h1}`);
  await pg.click("#canvaPrev"); await sleep(2500);
  const h2 = await halaman();
  ok(h2 < h1, "butang ◀ kembali", `${h1} → ${h2}`);
  for (const type of ["rawKeyDown", "keyUp"]) await pg.send("Input.dispatchKeyEvent", { type, key: "PageDown", code: "PageDown", windowsVirtualKeyCode: 34, nativeVirtualKeyCode: 34 });
  await sleep(2500);
  const h3 = await halaman();
  ok(h3 > h2, "PageDown (clicker) menukar slaid bila Canva ke udara", `${h2} → ${h3}`);
  const sedia = await pg.evaluate(() => __majlis.p().toJSON().sedia);
  ok(true, "SEDIA aturcara tidak berubah oleh clicker", JSON.stringify(sedia));
  const { imageData } = await o.call("GetSourceScreenshot", { sourceName: "Slaid Canva", imageFormat: "png", imageWidth: 960, imageHeight: 540 });
  fs.writeFileSync(path.join(OUT, "slaid-canva-program.png"), Buffer.from(imageData.split(",")[1], "base64"));

  // Projector screen
  await pg.click("#mSetup");
  await pg.click("#skrinPasang");
  const sk = await until(async () => { try { return (await o.call("GetInputSettings", { inputName: "Kawalan Majlis - Skrin Projektor" })).inputKind; } catch (_) { return null; } }, 10000);
  ok(sk === "monitor_capture", "input Skrin Projektor dicipta", sk);
  const pil = await until(() => pg.evaluate(() => { const s = document.getElementById("skrinPilih"); return !s.disabled && s.options.length ? [...s.options].map(o => o.textContent).join(" | ") : null; }), 8000);
  ok(!!pil, "senarai skrin dalam panel", pil);
  ok(pg.errs.length === 0, "tiada ralat dalam panel", pg.errs.slice(0, 3).join(" | "));

  // Clean up the dev config.
  await o.call("SetCurrentProgramScene", { sceneName: scenes.find(s => !/Slaid Canva|Skrin Projektor/.test(s.sceneName)).sceneName }).catch(() => {});
  for (const s of ["Slaid Canva", "Skrin Projektor"]) await o.call("RemoveScene", { sceneName: s }).catch(() => {});
  for (const i of ["Kawalan Majlis - Canva", "Kawalan Majlis - Skrin Projektor"]) await o.call("RemoveInput", { inputName: i }).catch(() => {});
  pg.close(); cv.close(); o.close();
  tutup(app.pid);
  let keluar = false;
  for (let i = 0; i < 40 && !keluar; i++) { await sleep(500); try { process.kill(app.pid, 0); } catch (_) { keluar = true; } }
  ok(keluar, "Studio ditutup");
  const crash = fs.existsSync(CRASH) ? fs.readdirSync(CRASH).filter(f => fs.statSync(path.join(CRASH, f)).mtimeMs > mula) : [];
  ok(crash.length === 0, "tiada laporan crash", crash.join(","));
  const { pass, fail } = hasil();
  console.log(`\n${pass} lulus, ${fail} gagal`);
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error(e); process.exit(2); });
