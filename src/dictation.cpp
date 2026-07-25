#include "dictation.h"
#include "denoise.h"
#include "transcriber.h"
#include <QApplication>
#include <QClipboard>
#include <chrono>
#include <vector>

Dictation::Dictation(const std::string &model_path, QObject *parent)
    : QObject(parent), m_model_path(model_path), m_streaming(false),
      m_stopping(false), m_notice(nullptr), m_state("listening"), m_level(0.0),
      m_fallback(false) {
    m_poll.setInterval(45);
    connect(&m_poll, &QTimer::timeout, this, &Dictation::poll);
    m_recorder.on_finished([this] {
        QMetaObject::invokeMethod(this, "stopListening", Qt::QueuedConnection);
    });
}
Dictation::~Dictation() {
    m_streaming.store(false);
    if (m_worker.joinable())
        m_worker.join();
    delete m_notice;
}
QString Dictation::state() const { return m_state; }
qreal Dictation::level() const { return m_level; }
QString Dictation::transcript() const { return m_transcript; }
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
        if (m_fallback)
            notice(QStringLiteral("Mikrofon bulunamadı"));
        QTimer::singleShot(2200, [] { QApplication::quit(); });
        return;
    }
    setState("listening");
    m_streaming.store(true);
    m_poll.start();
    m_worker = std::thread([this] { stream(); });
}
void Dictation::startFallback() {
    m_fallback = true;
    notice(QStringLiteral("Not defteri penceresi açılamadı. Metin panoya "
                          "kopyalanacak."));
    start();
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
void Dictation::stopListening() {
    if (m_stopping.exchange(true))
        return;
    m_poll.stop();
    m_recorder.stop();
    m_streaming.store(false);
    setState("transcribing");
}
void Dictation::stream() {
    if (m_ready.valid())
        m_ready.wait();
    while (m_streaming.load()) {
        std::vector<float> samples = m_recorder.snapshot();
        if (samples.size() >= 16000) {
            denoise(samples);
            std::string partial = transcribe(samples);
            if (!partial.empty())
                QMetaObject::invokeMethod(
                    this, "onPartial", Qt::QueuedConnection,
                    Q_ARG(QString, QString::fromStdString(partial)));
        }
        for (int i = 0; i < 8 && m_streaming.load(); i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::vector<float> samples = m_recorder.take();
    denoise(samples);
    std::string text = transcribe(samples);
    QMetaObject::invokeMethod(this, "onCompleted", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(text)));
}
void Dictation::onPartial(const QString &text) {
    m_transcript = text;
    emit transcriptChanged();
}
void Dictation::onCompleted(const QString &text) {
    if (!text.isEmpty()) {
        m_transcript = text;
        emit transcriptChanged();
    }
    setState("done");
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
void Dictation::quitNow() { QApplication::quit(); }
