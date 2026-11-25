/* Kawalan Majlis Studio: the "Bilik Kawalan" main window, laid out like docs/mockup-bilik-kawalan.html
 * (kawalan-majlis repo): top bar, Preview | transitions | Program as native OBS video, an input bar
 * of scenes, the Kawalan Majlis web panel (Panel Pengarah) through OBS's own CEF, and a native mixer. */
#pragma once

#include "OBSQTDisplay.hpp"

#include <obs.hpp>
#include <obs-frontend-api.h>
#include <util/platform.h>

#include <QElapsedTimer>
#include <QPointer>
#include <QWidget>
#include <mutex>
#include <vector>

class QComboBox;
class QFrame;
class QHBoxLayout;
class QBoxLayout;
class QLabel;
class QPushButton;
class QSlider;
class QTimer;
class QCefWidget;
class VolControl;

/* Called before modules load: obs-websocket on, with a random password (no Pasang.bat needed). */
void KMSediaWebSocket();
void KMSediakanNDI();
bool KMCanvaLangkah(int arah);

/* One native video box: the Preview scene, the Program output, or one scene. */
class KMDisplay : public OBSQTDisplay {
	Q_OBJECT

public:
	enum class Peranan { Preview, Program, Scene };

	KMDisplay(QWidget *parent, Peranan peranan, obs_source_t *scene = nullptr);
	~KMDisplay();

	void SetSource(obs_source_t *source);
	obs_source_t *Scene() const { return scene; }

signals:
	void Diklik();
	void DiklikDua();

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
	static void Render(void *data, uint32_t cx, uint32_t cy);

	Peranan peranan;
	obs_source_t *scene = nullptr; /* Peranan::Scene: the scene this tile shows */
	std::mutex lock;
	OBSWeakSourceAutoRelease weak;
	bool showing = false;
};

/* An input tile: the scene's video, its number and name, and a PROGRAM / PREVIEW tag. */
struct KMJubin {
	QFrame *bingkai;
	KMDisplay *video;
	QLabel *nama;
	QLabel *tag;
};

class BilikKawalan : public QWidget {
	Q_OBJECT

public:
	explicit BilikKawalan(QWidget *parent = nullptr);
	~BilikKawalan();

protected:
	void closeEvent(QCloseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

private:
	static void FrontendEvent(enum obs_frontend_event event, void *data);
	static void SourceSignal(void *data, calldata_t *cd);

	QWidget *BinaBarAtas();
	QWidget *BinaMonitor();
	QWidget *BinaInput();
	QWidget *BinaMixer();
	QWidget *BinaPanel();

	void SegarScene();
	void SegarPreview();
	void KemasTanda();
	void SegarMixer();
	void Pilih(obs_source_t *scene);
	void Peralihan(const char *jenis);
	void Ftb();
	void Detik();
	void KemasOutput();
	void Mesej(const QString &teks, bool buruk = false);
	void SetMod(bool majlis);
	void DaftarUji();
	void DaftarPanel();
	void *vendorPanel = nullptr; /* obs-websocket vendor "kawalan-majlis": the panel's own requests */
	void *vendor = nullptr; /* obs-websocket vendor for tests (KM_UJI=1) */
	bool menutup = false;   /* set first thing in the destructor: no timer or callback may run after */
	QTimer *detik = nullptr;

	/* top bar */
	QLabel *jam = nullptr, *statLabel = nullptr, *mesej = nullptr;
	QPushButton *recBtn = nullptr, *liveBtn = nullptr, *modPersediaan = nullptr, *modMajlis = nullptr;
	QTimer *tahanLive = nullptr, *mesejTimer = nullptr;
	QElapsedTimer masaRec, masaLive;
	os_cpu_usage_info_t *cpu = nullptr;
	uint64_t asasTercicir = 0;
	bool asasAda = false;

	/* monitors + transitions */
	KMDisplay *preview = nullptr, *program = nullptr;
	QLabel *pvNama = nullptr, *pgNama = nullptr, *pvMati = nullptr;
	QPushButton *studioBtn = nullptr, *ftbBtn = nullptr;
	QComboBox *tempoh = nullptr;
	QSlider *tbar = nullptr;
	OBSWeakSourceAutoRelease sebelumFtb;
	OBSWeakSourceAutoRelease pulihkanPv; /* Preview to put back once OBS finishes the FTB transition */

	/* inputs */
	QFrame *inputFrame = nullptr;
	QHBoxLayout *deretan = nullptr;
	std::vector<KMJubin> jubin;

	/* mixer */
	QBoxLayout *mixerRow = nullptr;
	std::vector<QPointer<VolControl>> saluran;
	bool mixerTertunda = false;

	QCefWidget *panel = nullptr;
	std::vector<OBSSignal> isyarat;
};
