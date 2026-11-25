// Shared settings for the live tests in km-ujian: paths relative to this repo, overridable by environment.
//   KM_RUNDIR   Studio folder to test (default: build_x64/rundir/RelWithDebInfo of this repo)
//   KM_UJI_OUT  where screenshots go (default: <temp>/km-ujian)
const path = require("path"), os = require("os"), fs = require("fs");
const { execFileSync } = require("child_process");

const ROOT = path.resolve(__dirname, "..");
const RD = process.env.KM_RUNDIR || path.join(ROOT, "build_x64", "rundir", "RelWithDebInfo");
const BIN = path.join(RD, "bin", "64bit");
const CONFIG = path.join(RD, "config", "obs-studio");
const CFG = path.join(CONFIG, "plugin_config", "obs-websocket", "config.json");
const CRASH = path.join(CONFIG, "crashes");
const LOGS = path.join(CONFIG, "logs");
const OUT = process.env.KM_UJI_OUT || path.join(os.tmpdir(), "km-ujian");
fs.mkdirSync(OUT, { recursive: true });

/** Close the Studio the way a person does (WM_CLOSE on its window), by process id only. */
function tutup(pid) {
  execFileSync("powershell", ["-NoProfile", "-ExecutionPolicy", "Bypass", "-File", path.join(__dirname, "tutup.ps1"), "-ProsesId", String(pid)], { stdio: "ignore" });
}

// The raw CDP / obs-websocket helpers live with the panel (submodule kawalan-majlis).
const cdp = require(path.join(ROOT, "kawalan-majlis", "ujian", "cdp.cjs"));

module.exports = Object.assign({ ROOT, RD, BIN, CONFIG, CFG, CRASH, LOGS, OUT, tutup }, cdp);
