// Live check in a configured Studio (KM_RUNDIR, with the panel installed in its scenes): MP3 album art never appears on screen (background music and songs),
// while the audio plays; an MP4 still shows its picture. Plays files through KM.dock.mainFail on a separate
// connection, so the aturcara state is not touched.
const path = require("path"), fs = require("fs");
const { spawn, execSync } = require("child_process");
const { obsSocket, sleep, until, tab, ok, hasil, BIN, CFG, CRASH, OUT, tutup } = require("./sama.cjs");
// KM_UJI_LAGU = an MP3 WITH album art, KM_UJI_VIDEO = an MP4, KM_UJI_SCENE = a scene holding the panel overlay
// that is otherwise black (a camera scene with no camera).
const LAGU = process.env.KM_UJI_LAGU, VIDEO = process.env.KM_UJI_VIDEO, SC = process.env.KM_UJI_SCENE;
if (!LAGU || !VIDEO || !SC) { console.log("LANGKAU: tetapkan KM_UJI_LAGU, KM_UJI_VIDEO dan KM_UJI_SCENE"); process.exit(0); }

// Mean brightness of a PNG screenshot region via ffmpeg (0 = black).
function terang(file) {
  const out = execSync(`ffmpeg -v error -i "${file}" -vf "scale=64:36,format=gray" -f rawvideo -`, { maxBuffer: 1 << 20 });
  return out.reduce((a, b) => a + b, 0) / out.length;
}

(async () => {
  const mula = Date.now();
  const app = spawn(path.join(BIN, "obs64.exe"), ["--portable", "--remote-debugging-port=9223", "--disable-shutdown-check", "--disable-updater"], { cwd: BIN, detached: true, stdio: "ignore" });
  let o = null;
  for (let i = 0; i < 60 && !o; i++) { await sleep(500); try { const c = JSON.parse(fs.readFileSync(CFG, "utf8")); o = await obsSocket(c.server_password, c.server_port); } catch (_) {} }
  ok(!!o, "Studio bermula");
  if (!o) process.exit(1);
  const asal = o.call.bind(o);
  o.call = async (t, d) => { for (let i = 0; ; i++) { try { return await asal(t, d); } catch (e) { if (!/not ready/i.test(e.message) || i > 40) throw e; await sleep(500); } } };
  const pg = await until(() => tab(/kawalan-majlis\.html/), 15000);
  await until(() => pg.evaluate(() => document.getElementById("kmConn").classList.contains("ok") ? 1 : null), 10000);
  await sleep(3000);
  const sebelum = await pg.evaluate(() => JSON.stringify(__majlis.p().toJSON()));

  const snap = async n => { const f = path.join(OUT, `album-${n}.png`); const { imageData } = await o.call("GetSourceScreenshot", { sourceName: SC, imageFormat: "png", imageWidth: 960, imageHeight: 540 }); fs.writeFileSync(f, Buffer.from(imageData.split(",")[1], "base64")); return terang(f); };

  // Background music: playing, but nothing on screen.
  await o.call("TriggerMediaInputAction", { inputName: "Kawalan Majlis - Muzik Latar", mediaAction: "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART" });
  await sleep(1500);
  const mz = await o.call("GetMediaInputStatus", { inputName: "Kawalan Majlis - Muzik Latar" });
  ok(mz.mediaState === "OBS_MEDIA_STATE_PLAYING", "muzik latar dimainkan");
  const rakam0 = async () => { await o.call("StartRecord"); await sleep(4000); const { outputPath } = await o.call("StopRecord"); await sleep(1500);
    const r = execSync(`ffmpeg -nostats -i "${outputPath}" -af volumedetect -vn -f null - 2>&1`).toString(); console.log("fail", outputPath, (/Stream.*Audio.*/.exec(r) || [""])[0]); fs.unlinkSync(outputPath); return parseFloat((/max_volume: (-?[0-9.]+) dB/.exec(r) || [0, "-99"])[1]); };
  console.log("muzik latar dalam rakaman:", await rakam0(), "dB");
  const ou = await o.call("GetRecordDirectory"); console.log(JSON.stringify(ou));
  console.log(JSON.stringify(await o.call("GetProfileParameter", { parameterCategory: "Output", parameterName: "Mode" })), JSON.stringify(await o.call("GetProfileParameter", { parameterCategory: "SimpleOutput", parameterName: "RecTracks" })), JSON.stringify(await o.call("GetProfileParameter", { parameterCategory: "Audio", parameterName: "SampleRate" })));
  console.log(JSON.stringify(await o.call("GetSpecialInputs")));
  const t0 = await snap("muzik");
  ok(t0 < 3, "kulit album muzik latar TIDAK kelihatan (scene kamera tanpa kamera = hitam)", "kecerahan " + t0.toFixed(1));

  // A song (MP3 with album art) and a video (MP4), on a separate connection.
  const pw = JSON.parse(fs.readFileSync(CFG, "utf8")).server_password;
  const main = laluan => pg.evaluate(async ([pw, laluan]) => {
    const t = new KM.OBSClient({ events: KM.SUB.General }); t.connect({ port: 4455, password: pw, retry: false });
    for (let i = 0; i < 40 && t.state !== "connected"; i++) await new Promise(r => setTimeout(r, 100));
    try { return await KM.dock.mainFail(t, laluan); } catch (e) { return e.message; } finally { t.disconnect(); }
  }, [pw, laluan]);
  await o.call("TriggerMediaInputAction", { inputName: "Kawalan Majlis - Muzik Latar", mediaAction: "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP" });
  await sleep(500);
  const tempoh = await main(LAGU);
  ok(typeof tempoh === "number" && tempoh > 0, "lagu MP3 dimainkan", String(tempoh));
  await sleep(1500);
  const lagu = await o.call("GetMediaInputStatus", { inputName: "Kawalan Majlis - Video" });
  ok(lagu.mediaState === "OBS_MEDIA_STATE_PLAYING" && lagu.mediaCursor > 0, "bunyi lagu berjalan", lagu.mediaState + " " + lagu.mediaCursor);
  // The audio really reaches the output: record 4 s (background music stopped) and measure the file.
  let rakamTerang = 0;
  const rakam = async () => {
    await o.call("StartRecord"); await sleep(4000);
    const { outputPath } = await o.call("StopRecord"); await sleep(1500);
    const r = execSync(`ffmpeg -nostats -i "${outputPath}" -af volumedetect -vn -f null - 2>&1`).toString();
    execSync(`ffmpeg -v error -y -ss 3 -i "${outputPath}" -frames:v 1 "${path.join(OUT, "rakaman-terakhir.png")}"`);
    rakamTerang = terang(path.join(OUT, "rakaman-terakhir.png"));
    fs.unlinkSync(outputPath);
    return parseFloat((/max_volume: (-?[\d.]+) dB/.exec(r) || [0, "-99"])[1]);
  };
  const db = await rakam();
  ok(db > -30, "bunyi lagu MP3 masuk ke rakaman/siaran", "puncak " + db + " dB");
  ok(rakamTerang > 15, "semasa lagu MP3, Program kekal (poster Skrin Tunggu, tiada kulit album)", "kecerahan " + rakamTerang.toFixed(1));
  fs.copyFileSync(path.join(OUT, "rakaman-terakhir.png"), path.join(OUT, "rakaman-lagu.png"));
  const t1 = await snap("lagu");
  ok(t1 < 3, "kulit album lagu TIDAK kelihatan", "kecerahan " + t1.toFixed(1));

  const tv = await main(VIDEO);
  ok(typeof tv === "number" && tv > 0, "video MP4 dimainkan");
  await sleep(2500);
  const t2 = await snap("video");
  ok(t2 > 15, "gambar video KELIHATAN", "kecerahan " + t2.toFixed(1));
  const db2 = await rakam();
  ok(db2 > -30, "bunyi video MP4 masuk ke rakaman (kawalan)", "puncak " + db2 + " dB");
  ok(rakamTerang > 15, "gambar video ada dalam rakaman Program (Skrin Tunggu)", "kecerahan " + rakamTerang.toFixed(1));

  await o.call("TriggerMediaInputAction", { inputName: "Kawalan Majlis - Video", mediaAction: "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP" });
  await pg.evaluate(() => KM.dock.setVideoVisible ? 1 : 0);
  for (const s of (await o.call("GetSceneList")).scenes) {
    const id = await o.call("GetSceneItemId", { sceneName: s.sceneName, sourceName: "Kawalan Majlis - Video" }).then(r => r.sceneItemId).catch(() => null);
    if (id) await o.call("SetSceneItemEnabled", { sceneName: s.sceneName, sceneItemId: id, sceneItemEnabled: false });
  }
  await o.call("TriggerMediaInputAction", { inputName: "Kawalan Majlis - Muzik Latar", mediaAction: "OBS_WEBSOCKET_MEDIA_INPUT_ACTION_STOP" });
  const selepas = await pg.evaluate(() => JSON.stringify(__majlis.p().toJSON()));
  ok(sebelum === selepas, "aturcara tidak berubah");
  ok(pg.errs.length === 0, "tiada ralat dalam panel", pg.errs.slice(0, 3).join(" | "));

  pg.close(); o.close();
  tutup(app.pid);
  let keluar = false;
  for (let i = 0; i < 40 && !keluar; i++) { await sleep(500); try { process.kill(app.pid, 0); } catch (_) { keluar = true; } }
  ok(keluar, "Studio ditutup");
  const cr = CRASH;
  ok(!(fs.existsSync(cr) && fs.readdirSync(cr).some(f => fs.statSync(cr + "/" + f).mtimeMs > mula)), "tiada laporan crash");
  const { pass, fail } = hasil();
  console.log(`\n${pass} lulus, ${fail} gagal`);
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error(e); process.exit(2); });
