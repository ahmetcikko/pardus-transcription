#pragma once
#include "recorder.h"
#include <QLabel>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <atomic>
#include <future>
#include <string>
#include <thread>

class Dictation : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(qreal level READ level NOTIFY levelChanged)
    Q_PROPERTY(QString transcript READ transcript NOTIFY transcriptChanged)
    Q_PROPERTY(bool turkish READ turkish WRITE setTurkish NOTIFY turkishChanged)
  public:
    explicit Dictation(const std::string &model_path,
                       QObject *parent = nullptr);
    ~Dictation();
    void start();
    void startFallback();
    QString state() const;
    qreal level() const;
    QString transcript() const;
    bool turkish() const;
    void setTurkish(bool value);
    Q_INVOKABLE void toggleListening();
    Q_INVOKABLE void clearText();
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE bool saveText(const QUrl &target, const QString &text);
    Q_INVOKABLE QString defaultFileName() const;
    Q_INVOKABLE void quitNow();

  signals:
    void stateChanged();
    void levelChanged();
    void transcriptChanged();
    void turkishChanged();

  private slots:
    void poll();
    void onPartial(const QString &text);
    void onPaused();

  private:
    void setState(const QString &value);
    void stream();
    void notice(const QString &text);
    Recorder m_recorder;
    QTimer m_poll;
    std::string m_model_path;
    std::future<void> m_ready;
    std::thread m_worker;
    std::atomic<bool> m_alive;
    std::atomic<bool> m_capturing;
    std::atomic<bool> m_flush;
    std::atomic<int> m_generation;
    QLabel *m_notice;
    QString m_state;
    QString m_transcript;
    qreal m_level;
    bool m_fallback;
    bool m_turkish;
};
