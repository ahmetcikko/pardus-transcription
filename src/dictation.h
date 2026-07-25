#pragma once
#include "recorder.h"
#include <QLabel>
#include <QObject>
#include <QString>
#include <QTimer>
#include <atomic>
#include <future>
#include <string>
#include <thread>

class Dictation : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(qreal level READ level NOTIFY levelChanged)
    Q_PROPERTY(QString transcript READ transcript NOTIFY transcriptChanged)
  public:
    explicit Dictation(const std::string &model_path,
                       QObject *parent = nullptr);
    ~Dictation();
    void start();
    void startFallback();
    QString state() const;
    qreal level() const;
    QString transcript() const;
    Q_INVOKABLE void stopListening();
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void quitNow();

  signals:
    void stateChanged();
    void levelChanged();
    void transcriptChanged();

  private slots:
    void poll();
    void onPartial(const QString &text);
    void onCompleted(const QString &text);

  private:
    void setState(const QString &value);
    void stream();
    void notice(const QString &text);
    Recorder m_recorder;
    QTimer m_poll;
    std::string m_model_path;
    std::future<void> m_ready;
    std::thread m_worker;
    std::atomic<bool> m_streaming;
    std::atomic<bool> m_stopping;
    QLabel *m_notice;
    QString m_state;
    QString m_transcript;
    qreal m_level;
    bool m_fallback;
};
