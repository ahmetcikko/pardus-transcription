#include "dictation.h"
#include "denoise.h"
#include "transcriber.h"
#include <QApplication>
#include <QClipboard>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <vector>

static constexpr int FALLBACK_MS = 15000;

Dictation::Dictation(const std::string &model_path, QObject *parent)
    : QObject(parent), m_model_path(model_path), m_alive(false),
      m_capturing(false), m_flush(false), m_generation(0), m_notice(nullptr),
      m_state("listening"), m_level(0.0), m_fallback(false), m_turkish(true) {
    m_poll.setInterval(45);
    connect(&m_poll, &QTimer::timeout, this, &Dictation::poll);
    m_recorder.on_full([this] {
        QMetaObject::invokeMethod(this, "toggleListening",
                                  Qt::QueuedConnection);
    });
}

Dictation::~Dictation() {
    m_alive.store(false);
    if (m_worker.joinable())
        m_worker.join();
    delete m_notice;
}

QString Dictation::state() const { return m_state; }

qreal Dictation::level() const { return m_level; }

QString Dictation::transcript() const { return m_transcript; }

bool Dictation::turkish() const { return m_turkish; }

void Dictation::setTurkish(bool value) {
    if (m_turkish == value)
        return;
    m_turkish = value;
    emit turkishChanged();
}

void Dictation::setState(const QString &value) {
    if (m_state == value)
        return;
    m_state = value;
    emit stateChanged();
}

void Dictation::start() {
    m_ready = std::async(std::launch::async,
                         [this] { transcriber_init(m_model_path); });
    if (!m_recorder.start()) {
        setState("error");
        if (m_fallback) {
            notice(QStringLiteral("Mikrofon bulunamadı"));
            QTimer::singleShot(2200, [] { QApplication::quit(); });
        }
        return;
    }

    setState("listening");
    m_alive.store(true);
    m_capturing.store(true);
    m_poll.start();
    m_worker = std::thread([this] { stream(); });
}

void Dictation::startFallback() {
    m_fallback = true;
    notice(QStringLiteral("Not defteri penceresi açılamadı. Metin panoya "
                          "kopyalanacak."));
    start();
    if (m_state == "listening")
        QTimer::singleShot(FALLBACK_MS, this, &Dictation::toggleListening);
}

void Dictation::notice(const QString &text) {
    if (!m_notice) {
        m_notice = new QLabel;
        (*m_notice).setWindowFlags(Qt::FramelessWindowHint |
                                   Qt::WindowStaysOnTopHint | Qt::Tool);
        (*m_notice).setAttribute(Qt::WA_ShowWithoutActivating);
        (*m_notice).setMargin(16);
        (*m_notice).setStyleSheet("background: #16171c; color: #eceef3;"
                                  " font-size: 14px;");
    }
    (*m_notice).setText(text);
    (*m_notice).adjustSize();
    (*m_notice).show();
}

void Dictation::poll() {
    m_level = m_recorder.level();
    emit levelChanged();
}

void Dictation::toggleListening() {
    if (m_state == "listening") {
        m_recorder.pause();
        m_capturing.store(false);
        m_flush.store(true);
        setState("transcribing");
    } else if (m_state == "paused") {
        if (!m_recorder.resume())
            return;
        m_capturing.store(true);
        setState("listening");
    }
}

void Dictation::clearText() {
    m_generation.fetch_add(1);
    m_recorder.clear();
    m_transcript.clear();
    emit transcriptChanged();
}

void Dictation::stream() {
    if (m_ready.valid())
        m_ready.wait();

    std::size_t last = 0;
    int generation = m_generation.load();
    while (m_alive.load()) {
        bool flush = m_flush.exchange(false);
        if (generation != m_generation.load()) {
            generation = m_generation.load();
            last = 0;
        }
        std::vector<float> samples = m_recorder.snapshot();
        if (samples.size() > last && (flush || m_capturing.load())) {
            last = samples.size();
            denoise(samples);
            std::string text = transcribe(samples);
            if (!text.empty() && generation == m_generation.load())
                QMetaObject::invokeMethod(
                    this, "onPartial", Qt::QueuedConnection,
                    Q_ARG(QString, QString::fromStdString(text)));
        }
        if (flush)
            QMetaObject::invokeMethod(this, "onPaused", Qt::QueuedConnection);

        for (int i = 0; i < 8 && m_alive.load() && !m_flush.load(); i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Dictation::onPartial(const QString &text) {
    m_transcript = text;
    emit transcriptChanged();
}

void Dictation::onPaused() {
    setState("paused");
    if (m_fallback) {
        copyText(m_transcript);
        notice(m_transcript.isEmpty()
                   ? QStringLiteral("Ses anlaşılamadı.")
                   : QStringLiteral("Metin panoya kopyalandı."));
        QTimer::singleShot(2600, [] { QApplication::quit(); });
    }
}

void Dictation::copyText(const QString &text) {
    (*QApplication::clipboard()).setText(text);
}

QString Dictation::defaultFileName() const {
    const char *home = getenv("HOME");
    std::string folder = home ? home : ".";
    std::time_t now = std::time(nullptr);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", std::localtime(&now));
    std::string path = folder + "/dikte-" + stamp + ".txt";
    return QUrl::fromLocalFile(QString::fromStdString(path)).toString();
}

bool Dictation::saveText(const QUrl &target, const QString &text) {
    std::ofstream file(target.toLocalFile().toStdString());
    if (!file)
        return false;
    file << text.toStdString();
    if (!text.endsWith('\n'))
        file << '\n';
    file.close();
    return file.good();
}

void Dictation::quitNow() { QApplication::quit(); }
