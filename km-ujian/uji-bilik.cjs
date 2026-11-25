// Kawalan Majlis Studio: live checks of the native Bilik Kawalan (no mouse used).
//   node km-ujian/uji-bilik.cjs
// Starts the portable build with KM_UJI=1, drives it through obs-websocket (+ vendor "kawalan-majlis-studio"),
// then closes it. Needs the test scenes created by the spike (Kamera Dewan, Kamera Pentas, Slaid, Logo).
const path = require("path"), fs = require("fs"), os = require("os");
const { spawn, execSync } = require("child_process");
const { obsSocket, sleep, until, ok, hasil, BIN, CFG, CRASH, tutup } = require("./sama.cjs");
let PW = null;
const HITAM = "Kawalan Majlis - Hitam";

const bilCrash = () => { try { return fs.readdirSync(CRASH).length; } catch (_) { return 0; } };

(async () => {
  const crashAwal = bilCrash();
  // Task 6: start with NO obs-websocket config: the app must create it (on, with a password) by itself.
  try { fs.unlinkSync(CFG); } catch (_) {}
  const env = Object.assign({}, process.env, { KM_UJI: "1" });
  delete env.KM_KLASIK; delete env.KM_PANEL;
  const app = spawn(path.join(BIN, "obs64.exe"), ["--portable", "--remote-debugging-port=9223", "--disable-shutdown-check", "--disable-updater"], { cwd: BIN, env, detached: true, stdio: "ignore" });
  let o = null;
  for (let i = 0; i < 40 && !o; i++) {
    await sleep(500);
    try { const c = JSON.parse(fs.readFileSync(CFG, "utf8")); PW = c.server_password; o = await obsSocket(PW, c.server_port); } catch (_) {}
  }
  ok(!!o, "aplikasi mencipta tetapan WebSocket sendiri dan WebSocket sedia");
  const cfg = o ? JSON.parse(fs.readFileSync(CFG, "utf8")) : {};
  ok(cfg.server_enabled === true && cfg.auth_required === true && String(cfg.server_password || "").length >= 16, "WebSocket: hidup, berkata laluan (16 aksara)");
  if (!o) process.exit(1);
  const v = async (requestType, requestData = {}) => (await o.call("CallVendorRequest", { vendorName: "kawalan-majlis-studio", requestType, requestData })).responseData;
  const k = await until(async () => { try { const s = await v("keadaan"); return s.jubin.length ? s : null; } catch (_) { return null; } }, 15000);
  ok(!!k, "Bilik Kawalan sedia (antara muka ujian menjawab)");
  if (!k) process.exit(1);
  const prog = async () => (await o.call("GetCurrentProgramScene")).currentProgramSceneName;
  const prev = async () => (await o.call("GetCurrentPreviewScene")).currentPreviewSceneName;
  const nama = k.jubin.map(j => j.nama);
  ok(["Kamera Dewan", "Kamera Pentas", "Slaid", "Logo"].every(n => nama.includes(n)) && !nama.includes(HITAM), "deretan input = scene OBS (tanpa scene Hitam)", nama.join(", "));
  ok(k.panel, "panel Kawalan Majlis dibenam");
  ok(k.mixer.length >= 1 && !k.mixer.some(m => /Paparan/.test(m.nama)), "mixer asli: sumber bunyi tanpa paparan", k.mixer.map(m => m.nama).join(", "));
  ok(/Bingkai tercicir/.test(k.stat) && /^\d\d:\d\d:\d\d$/.test(k.jam), "bar atas: jam + bingkai tercicir + CPU", k.jam + " | " + k.stat.replace(/<[^>]+>/g, ""));

  // Studio Mode off → the Preview box offers a button.
  await o.call("SetStudioModeEnabled", { studioModeEnabled: false });
  await sleep(400);
  ok((await v("keadaan")).studioButangNampak, "Studio Mode mati: butang Hidupkan Studio Mode nampak");
  await v("klik", { sasaran: "studio" });
  ok(await until(async () => (await o.call("GetStudioModeEnabled")).studioModeEnabled ? 1 : null, 3000), "butang menghidupkan Studio Mode");
  await sleep(500);

  const n = i => nama[i - 1];
  await o.call("SetCurrentProgramScene", { sceneName: n(1) });
  await v("jubin", { no: 2 });
  await sleep(700);
  ok((await prev()) === n(2) && (await prog()) === n(1), "klik input = Preview sahaja", `${await prev()} / ${await prog()}`);
  const k2 = await v("keadaan");
  ok(k2.jubin[1].keadaan === "pv" && k2.jubin[0].keadaan === "pg" && k2.jubin[1].tag === "PREVIEW", "tanda oren PREVIEW / merah PROGRAM pada input", JSON.stringify(k2.jubin.map(j => j.keadaan)));
  await v("klik", { sasaran: "cut" });
  ok(await until(async () => (await prog()) === n(2) ? 1 : null, 3000), "CUT: Preview ke Program");

  await v("kekunci", { kunci: "3" });
  ok(await until(async () => (await prev()) === n(3) ? 1 : null, 3000), "kekunci 3 = input 3 ke Preview");
  await v("kekunci", { kunci: "Enter" });
  ok(await until(async () => (await prog()) === n(3) ? 1 : null, 3000), "Enter = CUT");
  await v("kekunci", { kunci: "1" });
  await until(async () => (await prev()) === n(1) ? 1 : null, 3000);
  await v("kekunci", { kunci: "F" });
  const fOk = await until(async () => (await prog()) === n(1) ? 1 : null, 4000);
  ok(fOk, "F = FADE", fOk ? "" : `PG ${await prog()} PV ${await prev()} peralihan ${JSON.stringify(await o.call("GetCurrentSceneTransition"))} studio ${(await o.call("GetStudioModeEnabled")).studioModeEnabled}`);
  ok((await o.call("GetCurrentSceneTransition")).transitionDuration === 500, "FADE guna tempoh dipilih (0.5 s)");

  await v("jubin", { no: 4, dua: true });
  ok(await until(async () => (await prog()) === n(4) ? 1 : null, 3000), "klik dua kali = terus ke Program");
  await sleep(800);

  // T-bar: half-way really mixes (transition in progress), release past the end completes.
  await v("jubin", { no: 2 });
  await until(async () => (await prev()) === n(2) ? 1 : null, 3000);
  for (const x of [200, 400, 512]) { await v("tbar", { nilai: x }); await sleep(150); }
  const kt = await v("keadaan");
  ok(kt.tbarAktif && kt.masaPeralihan > 0.35 && kt.masaPeralihan < 0.65, "T-bar separuh: campuran sebenar sedang berlaku (masa peralihan ~0.5)", String(kt.masaPeralihan));
  for (const x of [800, 1024]) { await v("tbar", { nilai: x }); await sleep(120); }
  await v("tbar", { lepas: true });
  ok(await until(async () => (await prog()) === n(2) && !(await v("keadaan")).tbarAktif ? 1 : null, 3000), "T-bar sampai hujung + lepas = peralihan selesai, Program bertukar");

  // FTB.
  await sleep(800);
  await v("jubin", { no: 3 });
  await until(async () => (await prev()) === n(3) ? 1 : null, 3000);
  const sebelum = await prog();
  await v("klik", { sasaran: "ftb" });
  ok(await until(async () => (await prog()) === HITAM ? 1 : null, 4000), "FTB: skrin hitam ke udara");
  ok(await until(async () => (await prev()) === n(3) ? 1 : null, 3000), "FTB: pilihan Preview kekal");
  ok((await v("keadaan")).ftb, "butang FTB menyala");
  await v("klik", { sasaran: "ftb" });
  ok(await until(async () => (await prog()) === sebelum ? 1 : null, 4000), "FTB lagi: kembali ke scene tadi", sebelum);
  ok(!(await v("keadaan")).jubin.some(j => j.nama === HITAM), "scene Hitam tidak masuk deretan input");

  // REC into a temporary folder.
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), "kms-"));
  const { recordDirectory: asal } = await o.call("GetRecordDirectory");
  await o.call("SetRecordDirectory", { recordDirectory: tmp.split(path.sep).join("/") });
  await v("klik", { sasaran: "rec" });
  ok(await until(async () => (await o.call("GetRecordStatus")).outputActive ? 1 : null, 5000), "REC: rakaman bermula");
  await sleep(2200);
  ok(/REC\s+00:00:0[1-9]/.test((await v("keadaan")).rec), "REC menunjuk masa berjalan", (await v("keadaan")).rec);
  await v("klik", { sasaran: "rec" });
  ok(await until(async () => !(await o.call("GetRecordStatus")).outputActive ? 1 : null, 5000), "REC lagi: rakaman berhenti");
  await o.call("SetRecordDirectory", { recordDirectory: asal });

  // LIVE without a running stream: a click never stops anything; stopping needs a 2 s hold (checked in code review).
  ok(!(await o.call("GetStreamStatus")).outputActive, "siaran tidak dimulakan dalam ujian (tiada tetapan siaran)");

  // Persediaan / Majlis switch reaches the web panel.
  await v("klik", { sasaran: "persediaan" });
  const pages = await (await fetch("http://127.0.0.1:9223/json")).json();
  const pg = pages.find(p => /kawalan-majlis\.html/.test(p.url));
  ok(!!pg, "panel web boleh dicapai melalui CDP");
  ok(pg && /data\/obs-studio\/kawalan-majlis\/kawalan-majlis\.html\?tertanam=1&pw=/.test(decodeURIComponent(pg.url)), "panel dari fail terbina dalam aplikasi, mod tertanam", pg && decodeURIComponent(pg.url).replace(/pw=\w+/, "pw=***").slice(-90));
  if (pg) {
    const { Tab } = require("./sama.cjs");
    const t = await new Tab(pg).open();
    const conn = await until(() => t.evaluate(() => { const m = document.getElementById("kmConnMsg").textContent; return /^Bersambung/.test(m) ? m : null; }), 8000);
    ok(!!conn, "panel bersambung sendiri tanpa menaip kata laluan", conn || await t.evaluate(() => document.getElementById("kmConnMsg").textContent));
    const papar = await until(() => t.evaluate(() => getComputedStyle(document.getElementById("setup")).display !== "none" ? 1 : null), 3000);
    ok(!!papar, "butang ✎ Persediaan menukar panel ke Persediaan");
    const modesNampak = await t.evaluate(() => getComputedStyle(document.querySelector(".modes")).display !== "none");
    ok(!modesNampak, "butang mod web disembunyikan (diganti bar atas asli)");
    await v("klik", { sasaran: "majlis" });
    ok(await until(() => t.evaluate(() => getComputedStyle(document.getElementById("show")).display !== "none" ? 1 : null), 3000), "▶ Majlis kembali ke mod Majlis");
    ok(t.errs.length === 0, "tiada ralat JavaScript dalam panel", t.errs.slice(0, 2).join(" | "));
    t.close();
  }

  const s = await o.call("GetStats");
  ok(s.activeFps > 29 && s.cpuUsage < 25, "OBS lancar", `fps ${s.activeFps.toFixed(1)}, CPU ${s.cpuUsage.toFixed(1)}%`);
  ok(execSync(`powershell -NoProfile -Command "(Get-Process -Id ${app.pid}).Responding"`).toString().trim() === "True", "aplikasi responsif");

  o.close();
  tutup(app.pid);   // the X on the window, like a person
  let keluar = false;
  for (let i = 0; i < 40 && !keluar; i++) { await sleep(500); try { process.kill(app.pid, 0); } catch (_) { keluar = true; } }
  ok(keluar, "aplikasi keluar selepas ditutup");
  if (!keluar) execSync(`taskkill /F /PID ${app.pid}`, { stdio: "ignore" });   // our own test process only
  ok(bilCrash() === crashAwal, "tiada laporan crash semasa tutup", `${bilCrash() - crashAwal} baharu`);
  try { fs.rmSync(tmp, { recursive: true, force: true }); } catch (_) {}
  const { pass, fail } = hasil();
  console.log(`\n${pass} lulus, ${fail} gagal`);
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error(e); process.exit(2); });
