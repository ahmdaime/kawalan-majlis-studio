/* Kawalan Majlis Studio: the "Bilik Kawalan" main window. See BilikKawalan.hpp. */
#include "BilikKawalan.hpp"
#include "VolControl.hpp"

#include <components/Multiview.hpp>
#include <docks/BrowserDock.hpp>
#include <utility/display-helpers.hpp>
#include <utility/platform.hpp>
#include <OBSApp.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif
#include "../../plugins/obs-websocket/lib/obs-websocket-api.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <curl/curl.h>
#include <QComboBox>
#include <QDateTime>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSlider>
#include <QScrollArea>
#include <QSplitter>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

static const char *HITAM = "Kawalan Majlis - Hitam";
static const char *PAPARAN = "Kawalan Majlis - Paparan";

/* Colours of the approved mockup. */
static const char *GAYA = R"(
BilikKawalan { background:#111216; }
QWidget { color:#eceef2; font-family:"Segoe UI"; font-size:13px; }
QFrame#kmKad { background:#1b1d23; border:1px solid #30333c; border-radius:10px; }
QLabel#kmJenama { font-weight:800; font-size:15px; }
QLabel#kmJenamaKecil { color:#9aa0ad; font-weight:600; }
QLabel#kmJam { font-weight:800; font-size:16px; }
QLabel#kmStat, QLabel#kmPetunjuk { color:#9aa0ad; font-size:12px; }
QLabel#kmMesej { color:#ffcf7a; font-weight:700; }
QLabel#kmMesej[buruk="true"] { color:#ff9a9d; }
QLabel#kmKepala { color:#9aa0ad; font-weight:800; font-size:11px; letter-spacing:1px; }
QLabel#kmPv { background:#f08a24; color:#2a1300; font-weight:800; padding:5px 10px; border-top-left-radius:8px; border-top-right-radius:8px; }
QLabel#kmPg { background:#e5484d; color:#ffffff; font-weight:800; padding:5px 10px; border-top-left-radius:8px; border-top-right-radius:8px; }
QFrame#kmMonPv { border:2px solid #f08a24; border-radius:10px; background:#000; }
QFrame#kmMonPg { border:2px solid #e5484d; border-radius:10px; background:#000; }
QLabel#kmMati { color:#9aa0ad; background:#000; }
QPushButton { background:#23262d; border:1px solid #30333c; border-radius:8px; padding:8px 12px; font-weight:700; }
QPushButton:hover { background:#2b2f38; }
QPushButton:disabled { color:#666c7a; }
QPushButton#kmCut { background:#e9ecf1; color:#111111; border-color:#e9ecf1; font-size:16px; font-weight:900; padding:8px; }
QPushButton#kmFade { background:#3b5bdb; border-color:#3b5bdb; font-size:16px; font-weight:900; padding:8px; }
QPushButton#kmFtb { font-weight:900; padding:10px; }
QPushButton#kmFtb:checked { background:#e5484d; border-color:#e5484d; }
QPushButton#kmOut { font-weight:800; padding:7px 14px; }
QPushButton#kmOut[hidup="true"] { background:#e5484d; border-color:#e5484d; }
QPushButton#kmMod { padding:6px 12px; }
QPushButton#kmMod:checked { background:#eceef2; color:#15161a; border-color:#eceef2; }
QComboBox { background:#23262d; border:1px solid #30333c; border-radius:5px; padding:3px 6px; }
QSlider::groove:vertical { background:#0b0c0f; width:8px; border-radius:4px; }
QSlider::handle:vertical { background:#3b5bdb; height:22px; margin:0 -8px; border-radius:6px; }
QFrame#kmJubin { background:#23262d; border:2px solid #30333c; border-radius:8px; }
QFrame#kmJubin[keadaan="pv"] { border-color:#f08a24; }
QFrame#kmJubin[keadaan="pg"] { border-color:#e5484d; }
QLabel#kmTag { font-size:10px; font-weight:800; border-radius:3px; padding:1px 5px; }
QLabel#kmTag[keadaan="pv"] { background:#f08a24; color:#2a1300; }
QLabel#kmTag[keadaan="pg"] { background:#e5484d; color:#ffffff; }
QSplitter::handle { background:#111216; }
)";

static void Segar(QWidget *w)
{
	w->style()->unpolish(w);
	w->style()->polish(w);
}

/* ---------------- obs-websocket, set up by the app itself ---------------- */

static std::string KMFailWebSocket()
{
	char path[512];
	if (GetAppConfigPath(path, sizeof(path), "obs-studio/plugin_config/obs-websocket") <= 0)
		return std::string();
	os_mkdirs(path);
	return std::string(path) + "/config.json";
}

void KMSediaWebSocket()
{
	std::string f = KMFailWebSocket();
	if (f.empty())
		return;
	OBSDataAutoRelease d = obs_data_create_from_json_file_safe(f.c_str(), "bak");
	if (!d)
		d = obs_data_create();
	bool ubah = false;
	if (!obs_data_get_bool(d, "server_enabled")) {
		obs_data_set_bool(d, "server_enabled", true);
		ubah = true;
	}
	if (!obs_data_get_bool(d, "auth_required")) {
		obs_data_set_bool(d, "auth_required", true);
		ubah = true;
	}
	if (strlen(obs_data_get_string(d, "server_password")) < 8) {
		static const char abjad[] = "abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";
		std::string pw;
		for (int i = 0; i < 16; i++)
			pw += abjad[QRandomGenerator::system()->bounded(int(sizeof(abjad) - 1))];
		obs_data_set_string(d, "server_password", pw.c_str());
		ubah = true;
	}
	if (!obs_data_get_int(d, "server_port")) {
		obs_data_set_int(d, "server_port", 4455);
		ubah = true;
	}
	if (obs_data_get_bool(d, "first_load") || !obs_data_has_user_value(d, "first_load")) {
		obs_data_set_bool(d, "first_load", false);
		ubah = true;
	}
	if (ubah)
		obs_data_save_json_safe(d, f.c_str(), "tmp", "bak");
}

#ifdef _WIN32
/* NDI Runtime location from the machine environment in the registry. Programs started
   from an Explorer that predates the NDI install do not inherit NDI_RUNTIME_DIR_V6. */
static std::wstring KMNdiDariRegistri()
{
	wchar_t buf[1024];
	DWORD sz = sizeof(buf);
	if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
			 L"NDI_RUNTIME_DIR_V6", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, buf, &sz) != ERROR_SUCCESS)
		return std::wstring();
	wchar_t out[1024];
	DWORD n = ExpandEnvironmentStringsW(buf, out, 1024);
	return (n && n <= 1024) ? std::wstring(out) : std::wstring(buf);
}

static bool KMNdiAda(const std::wstring &dir)
{
	if (dir.empty())
		return false;
	std::wstring dll = dir + L"\\Processing.NDI.Lib.x64.dll";
	DWORD a = GetFileAttributesW(dll.c_str());
	return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static void KMNdiTetapkan(const std::wstring &dir)
{
	SetEnvironmentVariableW(L"NDI_RUNTIME_DIR_V6", dir.c_str());
	_wputenv_s(L"NDI_RUNTIME_DIR_V6", dir.c_str());
}

/* The bundled signed installer: <Studio>/pemasang/NDI*.exe (or the MASEP folder's 07 Pemasang). */
static QString KMNdiPemasang()
{
	QDir d(QCoreApplication::applicationDirPath());
	for (const char *rel : {"../../pemasang", "../../../07 Pemasang"}) {
		QDir p(d.absoluteFilePath(rel));
		for (const QString &f : p.entryList({"NDI*.exe"}, QDir::Files))
			return p.absoluteFilePath(f);
	}
	return QString();
}
#endif

void KMSediakanNDI()
{
#ifdef _WIN32
	if (qEnvironmentVariableIsSet("KM_TANPA_NDI")) /* tests: simulate a PC without NDI */
		return;
	wchar_t cur[1024];
	DWORD n = GetEnvironmentVariableW(L"NDI_RUNTIME_DIR_V6", cur, 1024);
	if (n && n < 1024 && KMNdiAda(cur))
		return;
	std::wstring reg = KMNdiDariRegistri();
	if (KMNdiAda(reg)) {
		KMNdiTetapkan(reg);
		blog(LOG_INFO, "[Kawalan Majlis] NDI Runtime dijumpai melalui registri");
		return;
	}

	QString pemasang = KMNdiPemasang();
	QMessageBox mb;
	mb.setIcon(QMessageBox::Information);
	mb.setWindowTitle("Kawalan Majlis Studio - Kamera NDI");
	mb.setText("<b>Perisian NDI belum dipasang pada komputer ini.</b>");
	mb.setInformativeText(pemasang.isEmpty()
				      ? "Kamera PTZ yang bersambung melalui rangkaian (NDI) tidak dapat dipaparkan.\n\n"
					"Muat turun \"NDI Tools\" (percuma, termasuk NDI Runtime) dari laman rasmi NDI, pasang, "
					"kemudian buka semula Studio. Bahagian lain (nama, lagu, video, slaid) tetap boleh digunakan."
				      : "Kamera PTZ yang bersambung melalui rangkaian (NDI) memerlukannya.\n\n"
					"Tekan \"Pasang NDI\" dan ikut arahan pemasang (klik Yes jika Windows bertanya). "
					"Studio akan terus dibuka selepas pemasangan selesai.");
	QPushButton *pasang = pemasang.isEmpty() ? nullptr : mb.addButton("Pasang NDI", QMessageBox::AcceptRole);
	/* No bundled installer (the public download may not redistribute it): open the official page. */
	QPushButton *laman = pemasang.isEmpty() ? mb.addButton("Buka laman muat turun NDI", QMessageBox::AcceptRole) : nullptr;
	mb.addButton("Teruskan tanpa kamera NDI", QMessageBox::RejectRole);
	mb.setDefaultButton(pasang ? pasang : laman);
	mb.exec();
	if (laman && mb.clickedButton() == laman) {
		QDesktopServices::openUrl(QUrl("https://ndi.video/tools/download/"));
		return;
	}
	if (!pasang || mb.clickedButton() != pasang)
		return;

	std::wstring exe = pemasang.toStdWString();
	SHELLEXECUTEINFOW sei = {sizeof(sei)};
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.lpVerb = L"open";
	sei.lpFile = exe.c_str();
	sei.nShow = SW_SHOWNORMAL;
	if (!ShellExecuteExW(&sei) || !sei.hProcess) {
		QMessageBox::warning(nullptr, "Kawalan Majlis Studio",
				     "Pemasang NDI tidak dapat dibuka. Buka sendiri fail ini:\n" + pemasang);
		return;
	}
	while (WaitForSingleObject(sei.hProcess, 100) == WAIT_TIMEOUT)
		QCoreApplication::processEvents();
	CloseHandle(sei.hProcess);
	reg = KMNdiDariRegistri();
	if (KMNdiAda(reg))
		KMNdiTetapkan(reg);
	else
		QMessageBox::information(nullptr, "Kawalan Majlis Studio",
					 "NDI belum dipasang sepenuhnya. Studio dibuka tanpa kamera NDI; "
					 "selepas NDI dipasang, buka semula Studio.");
#endif
}

/* The panel's own WebSocket connection: password + port from the same file. */
static QString KMQueryWebSocket()
{
	OBSDataAutoRelease d = obs_data_create_from_json_file_safe(KMFailWebSocket().c_str(), "bak");
	if (!d)
		return QString();
	int port = (int)obs_data_get_int(d, "server_port");
	return QString("&pw=%1&port=%2")
		.arg(QString::fromUtf8(QUrl::toPercentEncoding(QString::fromUtf8(obs_data_get_string(d, "server_password")))))
		.arg(port ? port : 4455);
}

/* ---------------- KMDisplay ---------------- */

KMDisplay::KMDisplay(QWidget *parent, Peranan peranan_, obs_source_t *scene_)
	: OBSQTDisplay(parent, Qt::Widget),
	  peranan(peranan_),
	  scene(scene_)
{
	setMinimumSize(120, 68);
	if (scene) {
		weak = OBSGetWeakRef(scene);
		obs_source_inc_showing(scene);
		showing = true;
	}
	connect(this, &OBSQTDisplay::DisplayCreated, [this]() {
		obs_display_add_draw_callback(GetDisplay(), Render, this);
		obs_display_set_background_color(GetDisplay(), 0x000000);
	});
}

KMDisplay::~KMDisplay()
{
	obs_display_remove_draw_callback(GetDisplay(), Render, this);
	if (showing) {
		OBSSourceAutoRelease s = obs_weak_source_get_source(weak);
		if (s)
			obs_source_dec_showing(s);
	}
}

void KMDisplay::SetSource(obs_source_t *source)
{
	std::lock_guard<std::mutex> g(lock);
	weak = source ? OBSGetWeakRef(source) : nullptr;
}

void KMDisplay::Render(void *data, uint32_t cx, uint32_t cy)
{
	KMDisplay *d = static_cast<KMDisplay *>(data);

	struct obs_video_info ovi;
	obs_get_video_info(&ovi);
	uint32_t targetCX = ovi.base_width, targetCY = ovi.base_height;

	int x, y;
	float scale;
	GetScaleAndCenterPos(targetCX, targetCY, cx, cy, x, y, scale);
	int newCX = int(scale * float(targetCX)), newCY = int(scale * float(targetCY));

	startRegion(x, y, newCX, newCY, 0.0f, float(targetCX), 0.0f, float(targetCY));
	if (d->peranan == Peranan::Program) {
		obs_render_main_texture();
	} else {
		OBSSourceAutoRelease s;
		{
			std::lock_guard<std::mutex> g(d->lock);
			s = obs_weak_source_get_source(d->weak);
		}
		if (s)
			obs_source_video_render(s);
	}
	endRegion();
}

void KMDisplay::mousePressEvent(QMouseEvent *event)
{
	OBSQTDisplay::mousePressEvent(event);
	if (event->button() == Qt::LeftButton)
		emit Diklik();
}

void KMDisplay::mouseDoubleClickEvent(QMouseEvent *event)
{
	OBSQTDisplay::mouseDoubleClickEvent(event);
	if (event->button() == Qt::LeftButton)
		emit DiklikDua();
}

/* ---------------- BilikKawalan ---------------- */

static QFrame *Kad(QWidget *parent)
{
	QFrame *f = new QFrame(parent);
	f->setObjectName("kmKad");
	return f;
}

static QLabel *Label(const QString &teks, const char *nama, QWidget *parent)
{
	QLabel *l = new QLabel(teks, parent);
	l->setObjectName(nama);
	return l;
}

BilikKawalan::BilikKawalan(QWidget *parent) : QWidget(parent, Qt::Window)
{
	setObjectName("BilikKawalan");
	setWindowTitle("Kawalan Majlis Studio");
	setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs.png")));
	setAttribute(Qt::WA_QuitOnClose, false);
	setStyleSheet(GAYA);
	resize(1480, 940);

	cpu = os_cpu_usage_info_start();

	QVBoxLayout *root = new QVBoxLayout(this);
	root->setContentsMargins(10, 10, 10, 10);
	root->setSpacing(10);
	root->addWidget(BinaBarAtas());

	/* Two columns so it fits a 1080p laptop at 125 % scaling (~790 px for the window):
	 * left = Preview | transitions | Program, inputs, audio; right = the Panel Pengarah, full height,
	 * so its yellow SEDIA box and PAPAR button are never pushed out of sight. */
	QSplitter *kiri = new QSplitter(Qt::Vertical, this);
	kiri->setChildrenCollapsible(false);
	kiri->addWidget(BinaMonitor());
	kiri->addWidget(BinaInput());
	kiri->addWidget(BinaMixer());
	kiri->setStretchFactor(0, 4);
	kiri->setStretchFactor(1, 2);
	kiri->setStretchFactor(2, 3);

	QWidget *panelW = BinaPanel();
	panelW->setMinimumWidth(440);
	QSplitter *split = new QSplitter(Qt::Horizontal, this);
	split->setChildrenCollapsible(false);
	split->addWidget(kiri);
	split->addWidget(panelW);
	split->setStretchFactor(0, 3);
	split->setStretchFactor(1, 2);
	root->addWidget(split, 1);

	QLabel *kunci = Label("Kekunci: 1–9 input ke Preview · Enter CUT · F FADE. Dalam panel Kawalan Majlis: Space butang "
			      "kuning · → Seterusnya · Esc SOROK. Klik pada bahagian yang mahu dikawal dahulu.",
			      "kmPetunjuk", this);
	root->addWidget(kunci);

	obs_frontend_add_event_callback(FrontendEvent, this);
	signal_handler_t *sh = obs_get_signal_handler();
	isyarat.emplace_back(sh, "source_create", SourceSignal, this);
	isyarat.emplace_back(sh, "source_destroy", SourceSignal, this);
	isyarat.emplace_back(sh, "source_rename", SourceSignal, this);

	detik = new QTimer(this);
	connect(detik, &QTimer::timeout, this, &BilikKawalan::Detik);
	detik->start(1000);

	SegarScene();
	SegarPreview();
	SegarMixer();
	KemasOutput();
	Detik();
	DaftarPanel();
	if (qEnvironmentVariableIsSet("KM_UJI"))
		DaftarUji();
}

/* Tear-down order matters: the web panel and video displays pump events while they close,
 * so every timer, callback and test request is stopped before anything is freed. */
BilikKawalan::~BilikKawalan()
{
	menutup = true;
	for (QTimer *t : {detik, tahanLive, mesejTimer})
		if (t)
			t->stop();
	obs_frontend_remove_event_callback(FrontendEvent, this);
	isyarat.clear();
	if (vendorPanel) {
		obs_websocket_vendor_unregister_request(vendorPanel, "canva");
		obs_websocket_vendor_unregister_request(vendorPanel, "pautan");
		vendorPanel = nullptr;
	}
	if (vendor) {
		for (const char *r : {"keadaan", "klik", "live", "jubin", "kekunci", "tbar"})
			obs_websocket_vendor_unregister_request(vendor, r);
		vendor = nullptr;
	}
	if (cpu) {
		os_cpu_usage_info_destroy(cpu);
		cpu = nullptr;
	}
}

/* ---------- top bar ---------- */

QWidget *BilikKawalan::BinaBarAtas()
{
	QFrame *bar = Kad(this);
	QHBoxLayout *l = new QHBoxLayout(bar);
	l->setContentsMargins(12, 8, 12, 8);
	l->setSpacing(12);
	l->addWidget(Label("Kawalan Majlis", "kmJenama", bar));
	l->addWidget(Label("Studio", "kmJenamaKecil", bar));
	mesej = Label("", "kmMesej", bar);
	l->addWidget(mesej, 1);
	jam = Label("--:--:--", "kmJam", bar);
	statLabel = Label("", "kmStat", bar);
	l->addWidget(jam);
	l->addWidget(statLabel);

	recBtn = new QPushButton("●  REC", bar);
	recBtn->setObjectName("kmOut");
	recBtn->setToolTip("Mula / henti rakaman");
	liveBtn = new QPushButton("●  LIVE", bar);
	liveBtn->setObjectName("kmOut");
	liveBtn->setToolTip("Mula siaran. Untuk henti: tekan dan tahan 2 saat.");
	l->addWidget(recBtn);
	l->addWidget(liveBtn);

	connect(recBtn, &QPushButton::clicked, this, []() {
		if (obs_frontend_recording_active())
			obs_frontend_recording_stop();
		else
			obs_frontend_recording_start();
	});

	/* LIVE: one click starts; stopping needs a 2-second hold, so a stray click never ends the stream. */
	tahanLive = new QTimer(this);
	tahanLive->setSingleShot(true);
	tahanLive->setInterval(2000);
	connect(tahanLive, &QTimer::timeout, this, [this]() {
		obs_frontend_streaming_stop();
		Mesej("Siaran dihentikan.");
	});
	connect(liveBtn, &QPushButton::pressed, this, [this]() {
		if (obs_frontend_streaming_active())
			tahanLive->start();
	});
	connect(liveBtn, &QPushButton::released, this, [this]() {
		if (tahanLive->isActive()) {
			tahanLive->stop();
			Mesej("Untuk henti siaran: tekan dan tahan butang LIVE selama 2 saat.", true);
		} else if (!obs_frontend_streaming_active()) {
			obs_frontend_streaming_start();
		}
	});

	modPersediaan = new QPushButton("✎  Persediaan", bar);
	modMajlis = new QPushButton("▶  Majlis", bar);
	for (QPushButton *b : {modPersediaan, modMajlis}) {
		b->setObjectName("kmMod");
		b->setCheckable(true);
		l->addWidget(b);
	}
	modMajlis->setChecked(true);
	connect(modPersediaan, &QPushButton::clicked, this, [this]() { SetMod(false); });
	connect(modMajlis, &QPushButton::clicked, this, [this]() { SetMod(true); });

	QPushButton *lanjut = new QPushButton("⚙", bar);
	lanjut->setToolTip("Tetapan lanjutan: buka tetingkap OBS penuh (sumber, kamera, tetapan siaran dan rakaman).");
	l->addWidget(lanjut);
	connect(lanjut, &QPushButton::clicked, this, []() { OBSBasic::Get()->KMTetapanLanjutan(); });

	mesejTimer = new QTimer(this);
	mesejTimer->setSingleShot(true);
	connect(mesejTimer, &QTimer::timeout, this, [this]() { mesej->clear(); });
	return bar;
}

void BilikKawalan::Mesej(const QString &teks, bool buruk)
{
	mesej->setText(teks);
	mesej->setProperty("buruk", buruk);
	Segar(mesej);
	mesejTimer->start(buruk ? 4000 : 2500);
}

/* Switch the embedded web panel between Persediaan and Majlis (its own rules decide, e.g. no Persediaan on air). */
void BilikKawalan::SetMod(bool majlis)
{
	if (panel)
		panel->executeJavaScript(majlis ? "document.getElementById('mShow').click()"
						: "document.getElementById('mSetup').click()");
	modMajlis->setChecked(majlis);
	modPersediaan->setChecked(!majlis);
}

static QString Masa(qint64 ms)
{
	qint64 s = ms / 1000;
	return QString("%1:%2:%3").arg(s / 3600, 2, 10, QChar('0')).arg((s / 60) % 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0'));
}

void BilikKawalan::Detik()
{
	if (menutup || OBSBasic::Get()->isClosing())
		return;
	jam->setText(QTime::currentTime().toString("HH:mm:ss"));
	uint64_t tercicir = (uint64_t)video_output_get_skipped_frames(obs_get_video()) + obs_get_lagged_frames();
	if (!asasAda) {
		asasTercicir = tercicir;
		asasAda = true;
	}
	double c = cpu ? os_cpu_usage_info_query(cpu) : 0.0;
	uint64_t n = tercicir - asasTercicir;
	statLabel->setText(QString("Bingkai tercicir: <b style='color:%1'>%2</b> · CPU %3%")
				   .arg(n ? "#ff9a9d" : "#3fb27f")
				   .arg(n)
				   .arg(c, 0, 'f', 1));
	KemasOutput();
}

void BilikKawalan::KemasOutput()
{
	if (menutup)
		return;
	bool rec = obs_frontend_recording_active(), live = obs_frontend_streaming_active();
	recBtn->setText(rec ? QString("●  REC  %1").arg(Masa(masaRec.isValid() ? masaRec.elapsed() : 0)) : "●  REC");
	liveBtn->setText(live ? QString("●  LIVE  %1").arg(Masa(masaLive.isValid() ? masaLive.elapsed() : 0)) : "●  LIVE");
	if (recBtn->property("hidup").toBool() != rec) {
		recBtn->setProperty("hidup", rec);
		Segar(recBtn);
	}
	if (liveBtn->property("hidup").toBool() != live) {
		liveBtn->setProperty("hidup", live);
		Segar(liveBtn);
	}
}

/* ---------- monitors + transitions ---------- */

QWidget *BilikKawalan::BinaMonitor()
{
	QWidget *w = new QWidget(this);
	QHBoxLayout *l = new QHBoxLayout(w);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(10);

	auto monitor = [this, w](const char *bingkai, const char *kepala, QLabel *&tajuk, KMDisplay *&video,
				 KMDisplay::Peranan p) {
		QFrame *f = new QFrame(w);
		f->setObjectName(bingkai);
		QVBoxLayout *v = new QVBoxLayout(f);
		v->setContentsMargins(0, 0, 0, 0);
		v->setSpacing(0);
		tajuk = Label("", kepala, f);
		tajuk->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); /* the header never grows */
		video = new KMDisplay(f, p);
		v->addWidget(tajuk);
		v->addWidget(video, 1);
		return f;
	};

	QFrame *pv = monitor("kmMonPv", "kmPv", pvNama, preview, KMDisplay::Peranan::Preview);
	pvMati = Label("Studio Mode belum dihidupkan.", "kmMati", pv);
	pvMati->setAlignment(Qt::AlignCenter);
	studioBtn = new QPushButton("Hidupkan Studio Mode", pv);
	QVBoxLayout *pvL = static_cast<QVBoxLayout *>(pv->layout());
	pvL->addWidget(pvMati);
	pvL->addWidget(studioBtn, 0, Qt::AlignCenter);
	connect(studioBtn, &QPushButton::clicked, this, []() { obs_frontend_set_preview_program_mode(true); });

	QFrame *tr = Kad(w);
	tr->setFixedWidth(150);
	QVBoxLayout *t = new QVBoxLayout(tr);
	t->setContentsMargins(10, 10, 10, 10);
	t->setSpacing(8);
	QLabel *h = Label("PERALIHAN", "kmKepala", tr);
	h->setAlignment(Qt::AlignCenter);
	t->addWidget(h);
	QPushButton *cut = new QPushButton("CUT", tr);
	cut->setObjectName("kmCut");
	cut->setToolTip("Enter");
	QPushButton *fade = new QPushButton("FADE", tr);
	fade->setObjectName("kmFade");
	fade->setToolTip("F");
	t->addWidget(cut);
	t->addWidget(fade);
	connect(cut, &QPushButton::clicked, this, [this]() { Peralihan("cut_transition"); });
	connect(fade, &QPushButton::clicked, this, [this]() { Peralihan("fade_transition"); });

	/* T-bar: drives OBS's own T-bar, so a half-way drag really mixes Preview into Program. */
	tbar = new QSlider(Qt::Vertical, tr);
	tbar->setRange(0, 1024);
	tbar->setInvertedAppearance(true);
	tbar->setToolTip("T-bar: tarik perlahan untuk peralihan manual");
	tbar->setMinimumHeight(70);
	t->addWidget(tbar, 1, Qt::AlignHCenter);
	connect(tbar, &QSlider::valueChanged, this, [this](int v) {
		OBSBasic *m = OBSBasic::Get();
		if (!obs_frontend_preview_program_mode_active() || !m->tBar)
			return;
		m->tBar->setValue(v * m->tBar->maximum() / 1024);
	});
	connect(tbar, &QSlider::sliderReleased, this, [this]() {
		OBSBasic *m = OBSBasic::Get();
		if (obs_frontend_preview_program_mode_active() && m->tBar)
			m->TBarReleased();
		tbar->blockSignals(true);
		tbar->setValue(0);
		tbar->blockSignals(false);
	});

	QHBoxLayout *d = new QHBoxLayout();
	d->addWidget(Label("Tempoh", "kmStat", tr));
	tempoh = new QComboBox(tr);
	tempoh->addItem("0.3 s", 300);
	tempoh->addItem("0.5 s", 500);
	tempoh->addItem("1 s", 1000);
	tempoh->addItem("2 s", 2000);
	tempoh->setCurrentIndex(1);
	d->addWidget(tempoh);
	t->addLayout(d);

	ftbBtn = new QPushButton("FTB", tr);
	ftbBtn->setObjectName("kmFtb");
	ftbBtn->setCheckable(true);
	ftbBtn->setToolTip("Skrin hitam ke penonton. Tekan lagi untuk kembali.");
	t->addWidget(ftbBtn);
	connect(ftbBtn, &QPushButton::clicked, this, [this]() { Ftb(); });

	QFrame *pg = monitor("kmMonPg", "kmPg", pgNama, program, KMDisplay::Peranan::Program);

	l->addWidget(pv, 1);
	l->addWidget(tr);
	l->addWidget(pg, 1);
	w->setMinimumHeight(tr->sizeHint().height()); /* the whole transition column, T-bar included, always fits */
	return w;
}

void BilikKawalan::Peralihan(const char *jenis)
{
	struct obs_frontend_source_list list = {};
	obs_frontend_get_transitions(&list);
	for (size_t i = 0; i < list.sources.num; i++) {
		if (strcmp(obs_source_get_id(list.sources.array[i]), jenis) == 0) {
			obs_frontend_set_current_transition(list.sources.array[i]);
			break;
		}
	}
	obs_frontend_source_list_free(&list);
	if (strcmp(jenis, "fade_transition") == 0)
		obs_frontend_set_transition_duration(tempoh->currentData().toInt());
	if (obs_frontend_preview_program_mode_active())
		obs_frontend_preview_program_trigger_transition();
	else
		Mesej("Hidupkan Studio Mode dahulu (butang dalam kotak Preview).", true);
}

/* FTB: fade to our black scene; pressed again, fade back to the scene that was on air. */
void BilikKawalan::Ftb()
{
	OBSSourceAutoRelease pg = obs_frontend_get_current_scene();
	bool diHitam = pg && strcmp(obs_source_get_name(pg), HITAM) == 0;
	OBSSourceAutoRelease sasaran;
	if (!diHitam) {
		sasaran = obs_get_source_by_name(HITAM);
		if (!sasaran) {
			obs_scene_t *s = obs_scene_create(HITAM);
			OBSDataAutoRelease set = obs_data_create();
			struct obs_video_info ovi;
			obs_get_video_info(&ovi);
			obs_data_set_int(set, "color", 0xff000000);
			obs_data_set_int(set, "width", ovi.base_width);
			obs_data_set_int(set, "height", ovi.base_height);
			OBSSourceAutoRelease warna = obs_source_create("color_source_v3", "Kawalan Majlis - Hitam (warna)", set, nullptr);
			obs_scene_add(s, warna);
			sasaran = obs_source_get_ref(obs_scene_get_source(s));
			obs_scene_release(s);
		}
		sebelumFtb = OBSGetWeakRef(pg);
	} else {
		sasaran = obs_weak_source_get_source(sebelumFtb);
		if (!sasaran && !jubin.empty())
			sasaran = obs_source_get_ref(jubin.front().video->Scene());
	}
	if (!sasaran)
		return;

	bool studio = obs_frontend_preview_program_mode_active();
	OBSSourceAutoRelease pvAsal = studio ? obs_frontend_get_current_preview_scene() : nullptr;
	Peralihan("fade_transition");
	if (studio) {
		/* Keep the operator's Preview choice: put it back when OBS reports the transition finished
		 * (OBS swaps Preview and Program at that moment, so a timer could lose the race). */
		if (pvAsal && strcmp(obs_source_get_name(pvAsal), HITAM) != 0)
			pulihkanPv = OBSGetWeakRef(pvAsal);
		obs_frontend_set_current_preview_scene(sasaran);
		obs_frontend_preview_program_trigger_transition();
	} else {
		obs_frontend_set_current_scene(sasaran);
	}
}

void BilikKawalan::SegarPreview()
{
	bool studio = obs_frontend_preview_program_mode_active();
	OBSSourceAutoRelease pv = studio ? obs_frontend_get_current_preview_scene() : nullptr;
	OBSSourceAutoRelease pg = obs_frontend_get_current_scene();
	preview->SetSource(pv);
	preview->setVisible(studio);
	pvMati->setVisible(!studio);
	studioBtn->setVisible(!studio);
	pvNama->setText(studio ? QString("PREVIEW · BELUM KELUAR      %1").arg(QT_UTF8(obs_source_get_name(pv)))
			       : QString("PREVIEW"));
	bool hitam = pg && strcmp(obs_source_get_name(pg), HITAM) == 0;
	pgNama->setText(QString("PROGRAM · KE UDARA      %1").arg(hitam ? QString("SKRIN HITAM") : QT_UTF8(obs_source_get_name(pg))));
	ftbBtn->setChecked(hitam);
	KemasTanda();
}

/* ---------- input bar ---------- */

QWidget *BilikKawalan::BinaInput()
{
	inputFrame = Kad(this);
	QVBoxLayout *v = new QVBoxLayout(inputFrame);
	v->setContentsMargins(10, 8, 10, 10);
	v->setSpacing(6);
	QHBoxLayout *h = new QHBoxLayout();
	h->addWidget(Label("INPUT", "kmKepala", inputFrame));
	h->addStretch(1);
	h->addWidget(Label("Klik = Preview · klik dua kali = terus ke Program · 1–9", "kmStat", inputFrame));
	v->addLayout(h);
	deretan = new QHBoxLayout();
	deretan->setSpacing(8);
	v->addLayout(deretan, 1);
	return inputFrame;
}

void BilikKawalan::Pilih(obs_source_t *scene)
{
	if (!scene)
		return;
	if (!obs_frontend_preview_program_mode_active()) {
		Mesej("Hidupkan Studio Mode dahulu (butang dalam kotak Preview).", true);
		return;
	}
	obs_frontend_set_current_preview_scene(scene);
}

void BilikKawalan::SegarScene()
{
	for (KMJubin &j : jubin)
		j.bingkai->deleteLater();
	jubin.clear();

	struct obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);
	int no = 0;
	for (size_t i = 0; i < scenes.sources.num; i++) {
		obs_source_t *s = scenes.sources.array[i];
		if (strcmp(obs_source_get_name(s), HITAM) == 0)
			continue;
		no++;
		QFrame *f = new QFrame(inputFrame);
		f->setObjectName("kmJubin");
		QVBoxLayout *v = new QVBoxLayout(f);
		v->setContentsMargins(2, 2, 2, 2);
		v->setSpacing(2);
		KMDisplay *d = new KMDisplay(f, KMDisplay::Peranan::Scene, s);
		QHBoxLayout *b = new QHBoxLayout();
		b->setContentsMargins(6, 0, 6, 2);
		QLabel *n = new QLabel(QString("<b>%1</b>  %2").arg(no).arg(QT_UTF8(obs_source_get_name(s)).toHtmlEscaped()), f);
		QLabel *tag = Label("", "kmTag", f);
		b->addWidget(n, 1);
		b->addWidget(tag);
		v->addWidget(d, 1);
		v->addLayout(b);
		deretan->addWidget(f, 1);
		connect(d, &KMDisplay::Diklik, this, [this, d]() { Pilih(d->Scene()); });
		connect(d, &KMDisplay::DiklikDua, this, [this, d]() {
			Pilih(d->Scene());
			Peralihan("cut_transition");
		});
		jubin.push_back({f, d, n, tag});
	}
	obs_frontend_source_list_free(&scenes);
	KemasTanda();
}

/* Red border + PROGRAM tag on the scene on air, orange + PREVIEW on the one queued. */
void BilikKawalan::KemasTanda()
{
	bool studio = obs_frontend_preview_program_mode_active();
	OBSSourceAutoRelease pv = studio ? obs_frontend_get_current_preview_scene() : nullptr;
	OBSSourceAutoRelease pg = obs_frontend_get_current_scene();
	for (KMJubin &j : jubin) {
		obs_source_t *s = j.video->Scene();
		const char *k = s == pg.Get() ? "pg" : s == pv.Get() ? "pv" : "";
		j.bingkai->setProperty("keadaan", k);
		j.tag->setProperty("keadaan", k);
		j.tag->setText(strcmp(k, "pg") == 0 ? "PROGRAM" : strcmp(k, "pv") == 0 ? "PREVIEW" : "");
		j.tag->setVisible(*k);
		Segar(j.bingkai);
		Segar(j.tag);
	}
}

void BilikKawalan::keyPressEvent(QKeyEvent *event)
{
	int k = event->key();
	if (!event->isAutoRepeat() && k >= Qt::Key_1 && k <= Qt::Key_9) {
		size_t i = size_t(k - Qt::Key_1);
		if (i < jubin.size())
			Pilih(jubin[i].video->Scene());
		return;
	}
	if (!event->isAutoRepeat() && (k == Qt::Key_Return || k == Qt::Key_Enter)) {
		Peralihan("cut_transition");
		return;
	}
	if (k == Qt::Key_PageDown || k == Qt::Key_PageUp) { /* presentation clicker */
		KMCanvaLangkah(k == Qt::Key_PageDown ? 1 : -1);
		return;
	}
	if (!event->isAutoRepeat() && k == Qt::Key_F) {
		Peralihan("fade_transition");
		return;
	}
	QWidget::keyPressEvent(event);
}

/* ---------- Slaid Canva: the browser source "Kawalan Majlis - Canva" gets an arrow key ----------
 * Canva's own viewer moves one step (animation or page) per arrow press. */
bool KMCanvaLangkah(int arah)
{
	OBSSourceAutoRelease src = obs_get_source_by_name("Kawalan Majlis - Canva");
	if (!src)
		return false;
	static char kosong[1] = "";
	obs_key_event e = {};
	e.text = kosong;
#ifdef _WIN32
	e.native_vkey = arah > 0 ? VK_RIGHT : VK_LEFT;
	e.native_scancode = arah > 0 ? 0x4D : 0x4B;
#endif
	obs_source_send_key_click(src, &e, false);
	obs_source_send_key_click(src, &e, true);
	return true;
}

/* A short share link (canva.link/...) -> the full canva.com/design/... address it redirects to.
   libcurl (OBS ships it with TLS; Qt's network module has no TLS backend in OBS). */
static size_t KMBuangData(char *, size_t sz, size_t n, void *)
{
	return sz * n;
}

static QString KMIkutPautan(const QString &mula)
{
	std::string cur = mula.toStdString();
	CURL *c = curl_easy_init();
	if (!c)
		return mula;
	for (int i = 0; i < 6 && cur.find("canva.com/design/") == std::string::npos; i++) {
		curl_easy_setopt(c, CURLOPT_URL, cur.c_str());
		curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 0L);
		curl_easy_setopt(c, CURLOPT_NOBODY, 0L);
		curl_easy_setopt(c, CURLOPT_TIMEOUT, 8L);
		curl_easy_setopt(c, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) KawalanMajlisStudio");
		curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, KMBuangData);
#ifdef _WIN32
		curl_easy_setopt(c, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_NATIVE_CA);
#endif
		if (curl_easy_perform(c) != CURLE_OK)
			break;
		char *ke = nullptr;
		if (curl_easy_getinfo(c, CURLINFO_REDIRECT_URL, &ke) != CURLE_OK || !ke || !*ke)
			break;
		cur = ke;
	}
	curl_easy_cleanup(c);
	return QString::fromStdString(cur);
}

void BilikKawalan::DaftarPanel()
{
	vendorPanel = obs_websocket_register_vendor("kawalan-majlis");
	if (!vendorPanel)
		return;
	obs_websocket_vendor_register_request(
		vendorPanel, "canva",
		[](obs_data_t *req, obs_data_t *res, void *) {
			int arah = (int)obs_data_get_int(req, "arah");
			obs_data_set_bool(res, "ok", KMCanvaLangkah(arah < 0 ? -1 : 1));
		},
		this);
	obs_websocket_vendor_register_request(
		vendorPanel, "pautan",
		[](obs_data_t *req, obs_data_t *res, void *) {
			QString url = QString::fromUtf8(obs_data_get_string(req, "url"));
			if (url.startsWith("https://", Qt::CaseInsensitive))
				url = KMIkutPautan(url);
			obs_data_set_string(res, "url", QT_TO_UTF8(url));
		},
		this);
}

/* ---------- mixer (OBS's own volume controls: smooth meters) ---------- */

QWidget *BilikKawalan::BinaMixer()
{
	QFrame *f = Kad(this);
	QVBoxLayout *v = new QVBoxLayout(f);
	v->setContentsMargins(10, 8, 10, 10);
	QHBoxLayout *h = new QHBoxLayout();
	h->addWidget(Label("AUDIO", "kmKepala", f));
	h->addStretch(1);
	h->addWidget(Label("Tukar input tidak menyentuh audio", "kmStat", f));
	v->addLayout(h);
	/* Horizontal faders stacked: full names, little height; scrolls when there are many sources. */
	QScrollArea *skrol = new QScrollArea(f);
	skrol->setWidgetResizable(true);
	skrol->setFrameShape(QFrame::NoFrame);
	skrol->setStyleSheet("QScrollArea, QScrollArea > QWidget > QWidget { background:transparent; }");
	QWidget *isi = new QWidget(skrol);
	mixerRow = new QVBoxLayout(isi);
	mixerRow->setContentsMargins(0, 0, 4, 0);
	mixerRow->setSpacing(2);
	skrol->setWidget(isi);
	v->addWidget(skrol, 1);
	f->setMinimumHeight(120);
	return f;
}

void BilikKawalan::SegarMixer()
{
	mixerTertunda = false;
	if (menutup || OBSBasic::Get()->isClosing())
		return;
	for (QPointer<VolControl> &c : saluran)
		if (c)
			c->deleteLater();
	saluran.clear();

	auto tambah = [](void *data, obs_source_t *s) {
		BilikKawalan *b = static_cast<BilikKawalan *>(data);
		uint32_t f = obs_source_get_output_flags(s);
		if (!(f & OBS_SOURCE_AUDIO) || obs_source_removed(s) || strcmp(obs_source_get_name(s), PAPARAN) == 0)
			return true;
		VolControl *c = new VolControl(OBSSource(s), false, false);
		b->mixerRow->addWidget(c);
		b->saluran.push_back(c);
		return true;
	};
	obs_enum_sources(tambah, this);
	mixerRow->addStretch(1);
}

/* ---------- web panel (Panel Pengarah) ---------- */

QWidget *BilikKawalan::BinaPanel()
{
	QFrame *f = Kad(this);
	QVBoxLayout *v = new QVBoxLayout(f);
	v->setContentsMargins(2, 2, 2, 2);
#ifdef BROWSER_AVAILABLE
	OBSBasic::InitBrowserPanelSafeBlock();
	if (cef) {
		/* The panel ships inside the app (data/obs-studio/kawalan-majlis). ?tertanam=1: the web panel hides its own
		 * mode buttons and large layout (the native window has them). KM_PANEL overrides it for development. */
		QString url;
		std::string bundel;
		if (qEnvironmentVariableIsSet("KM_PANEL"))
			url = qEnvironmentVariable("KM_PANEL");
		else if (GetDataFilePath("kawalan-majlis/kawalan-majlis.html", bundel))
			/* GetDataFilePath gives a path relative to the program folder: make it absolute for a file:// URL. */
			url = QUrl::fromLocalFile(QFileInfo(QString::fromStdString(bundel)).absoluteFilePath()).toString() +
			      "?tertanam=1" + KMQueryWebSocket();
		if (!url.isEmpty()) {
			panel = cef->create_widget(f, url.toUtf8().constData(), panel_cookies);
			v->addWidget(panel);
		}
	}
#endif
	if (!panel)
		v->addWidget(new QLabel("Panel Kawalan Majlis tidak tersedia (browser OBS tiada).", f));
	return f;
}

/* ---------- events ---------- */

void BilikKawalan::FrontendEvent(enum obs_frontend_event event, void *data)
{
	BilikKawalan *b = static_cast<BilikKawalan *>(data);
	if (b->menutup)
		return;
	switch (event) {
	case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		b->SegarScene();
		b->SegarMixer();
		b->SegarPreview();
		break;
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
	case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
		b->SegarPreview();
		break;
	case OBS_FRONTEND_EVENT_TRANSITION_STOPPED:
		if (b->pulihkanPv) {
			OBSSourceAutoRelease s = obs_weak_source_get_source(b->pulihkanPv);
			b->pulihkanPv = nullptr;
			if (s && obs_frontend_preview_program_mode_active())
				obs_frontend_set_current_preview_scene(s);
		}
		b->SegarPreview();
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		b->masaRec.start();
		b->KemasOutput();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		b->masaLive.start();
		b->KemasOutput();
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		b->KemasOutput();
		break;
	default:
		break;
	}
}

/* A source was added, removed or renamed: rebuild the mixer once, on the UI thread. */
void BilikKawalan::SourceSignal(void *data, calldata_t *)
{
	BilikKawalan *b = static_cast<BilikKawalan *>(data);
	if (b->menutup)
		return;
	QMetaObject::invokeMethod(
		b,
		[b]() {
			if (b->mixerTertunda)
				return;
			b->mixerTertunda = true;
			QTimer::singleShot(300, b, [b]() { b->SegarMixer(); });
		},
		Qt::QueuedConnection);
}

/* Closing the Bilik Kawalan closes the app (OBS asks first if it is streaming or recording). */
void BilikKawalan::closeEvent(QCloseEvent *event)
{
	event->ignore();
	OBSBasic *m = OBSBasic::Get();
	QTimer::singleShot(0, m, [m]() {
		m->kmKeluar = true;
		m->close();
		if (!m->isClosing())
			m->kmKeluar = false; /* the teacher cancelled */
	});
}

/* ---------- test interface (KM_UJI=1): obs-websocket vendor "kawalan-majlis-studio" ----------
 * Lets the tests press buttons, click inputs and read the window state without touching the real mouse. */
template<typename F> static void DiUI(BilikKawalan *b, F f)
{
	QMetaObject::invokeMethod(b, f, Qt::BlockingQueuedConnection);
}

void BilikKawalan::DaftarUji()
{
	vendor = obs_websocket_register_vendor("kawalan-majlis-studio");
	if (!vendor)
		return;
	obs_websocket_vendor_register_request(
		vendor, "keadaan",
		[](obs_data_t *, obs_data_t *res, void *p) {
			BilikKawalan *b = static_cast<BilikKawalan *>(p);
			DiUI(b, [b, res]() {
				obs_data_set_string(res, "pvTajuk", QT_TO_UTF8(b->pvNama->text()));
				obs_data_set_string(res, "pgTajuk", QT_TO_UTF8(b->pgNama->text()));
				obs_data_set_bool(res, "studioButangNampak", b->studioBtn->isVisible());
				obs_data_set_bool(res, "ftb", b->ftbBtn->isChecked());
				obs_data_set_string(res, "rec", QT_TO_UTF8(b->recBtn->text()));
				obs_data_set_string(res, "live", QT_TO_UTF8(b->liveBtn->text()));
				obs_data_set_string(res, "mesej", QT_TO_UTF8(b->mesej->text()));
				obs_data_set_string(res, "stat", QT_TO_UTF8(b->statLabel->text()));
				obs_data_set_string(res, "jam", QT_TO_UTF8(b->jam->text()));
				obs_data_set_bool(res, "panel", b->panel != nullptr);
				obs_data_set_bool(res, "tbarAktif", OBSBasic::Get()->tBarActive);
				OBSSourceAutoRelease tr = obs_get_output_source(0);
				obs_data_set_double(res, "masaPeralihan", tr ? obs_transition_get_time(tr) : -1.0);
				OBSDataArrayAutoRelease t = obs_data_array_create();
				for (KMJubin &j : b->jubin) {
					OBSDataAutoRelease o = obs_data_create();
					obs_data_set_string(o, "nama", obs_source_get_name(j.video->Scene()));
					obs_data_set_string(o, "keadaan", QT_TO_UTF8(j.bingkai->property("keadaan").toString()));
					obs_data_set_string(o, "tag", QT_TO_UTF8(j.tag->text()));
					obs_data_array_push_back(t, o);
				}
				obs_data_set_array(res, "jubin", t);
				OBSDataArrayAutoRelease m = obs_data_array_create();
				for (QPointer<VolControl> &c : b->saluran) {
					if (!c)
						continue;
					OBSDataAutoRelease o = obs_data_create();
					obs_data_set_string(o, "nama", obs_source_get_name(c->GetSource()));
					obs_data_array_push_back(m, o);
				}
				obs_data_set_array(res, "mixer", m);
			});
		},
		this);
	obs_websocket_vendor_register_request(
		vendor, "klik",
		[](obs_data_t *req, obs_data_t *res, void *p) {
			BilikKawalan *b = static_cast<BilikKawalan *>(p);
			std::string s = obs_data_get_string(req, "sasaran");
			DiUI(b, [b, s, res]() {
				QPushButton *btn = s == "rec" ? b->recBtn : s == "ftb" ? b->ftbBtn : s == "studio" ? b->studioBtn
					: s == "persediaan" ? b->modPersediaan : s == "majlis" ? b->modMajlis : nullptr;
				if (s == "cut" || s == "fade")
					b->Peralihan(s == "cut" ? "cut_transition" : "fade_transition");
				else if (btn)
					btn->click();
				obs_data_set_bool(res, "ok", btn || s == "cut" || s == "fade");
			});
		},
		this);
	obs_websocket_vendor_register_request(
		vendor, "live",
		[](obs_data_t *req, obs_data_t *res, void *p) {
			BilikKawalan *b = static_cast<BilikKawalan *>(p);
			int ms = (int)obs_data_get_int(req, "tahanMs");
			DiUI(b, [b, ms]() {
				emit b->liveBtn->pressed();
				QTimer::singleShot(ms, b, [b]() { emit b->liveBtn->released(); });
			});
			obs_data_set_bool(res, "ok", true);
		},
		this);
	obs_websocket_vendor_register_request(
		vendor, "jubin",
		[](obs_data_t *req, obs_data_t *res, void *p) {
			BilikKawalan *b = static_cast<BilikKawalan *>(p);
			size_t no = (size_t)obs_data_get_int(req, "no");
			bool dua = obs_data_get_bool(req, "dua");
			DiUI(b, [b, no, dua, res]() {
				bool ok = no >= 1 && no <= b->jubin.size();
				if (ok) {
					KMDisplay *d = b->jubin[no - 1].video;
					if (dua)
						emit d->DiklikDua();
					else
						emit d->Diklik();
				}
				obs_data_set_bool(res, "ok", ok);
			});
		},
		this);
	obs_websocket_vendor_register_request(
		vendor, "kekunci",
		[](obs_data_t *req, obs_data_t *, void *p) {
			BilikKawalan *b = static_cast<BilikKawalan *>(p);
			std::string k = obs_data_get_string(req, "kunci");
			DiUI(b, [b, k]() {
				int key = k == "Enter" ? Qt::Key_Return : k == "F" ? Qt::Key_F : (k.size() == 1 ? Qt::Key_0 + (k[0] - '0') : 0);
				QKeyEvent e(QEvent::KeyPress, key, Qt::NoModifier);
				QCoreApplication::sendEvent(b, &e);
			});
		},
		this);
	obs_websocket_vendor_register_request(
		vendor, "tbar",
		[](obs_data_t *req, obs_data_t *, void *p) {
			BilikKawalan *b = static_cast<BilikKawalan *>(p);
			int v = (int)obs_data_get_int(req, "nilai");
			bool lepas = obs_data_get_bool(req, "lepas");
			DiUI(b, [b, v, lepas]() {
				if (lepas)
					emit b->tbar->sliderReleased();
				else
					b->tbar->setValue(v);
			});
		},
		this);
}

#include "moc_BilikKawalan.cpp"
