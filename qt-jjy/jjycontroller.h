#pragma once

#include <QAudioSink>
#include <QByteArray>
#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QVector>
#include <memory>

#include "jjyaudiodevice.h"

class JjyController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool pending READ pending NOTIFY pendingChanged)
    Q_PROPERTY(bool summerTime READ summerTime WRITE setSummerTime NOTIFY summerTimeChanged)
    Q_PROPERTY(QString currentTime READ currentTime NOTIFY currentTimeChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QVariantList frame READ frame NOTIFY frameChanged)
    Q_PROPERTY(QVariantList bitDefinitions READ bitDefinitions CONSTANT)
    Q_PROPERTY(QVariantList decodedSummary READ decodedSummary NOTIFY frameChanged)
    Q_PROPERTY(int activeSecond READ activeSecond NOTIFY activeSecondChanged)

public:
    explicit JjyController(QObject *parent = nullptr);
    ~JjyController() override;

    bool running() const;
    bool pending() const;
    bool summerTime() const;
    void setSummerTime(bool enabled);
    QString currentTime() const;
    QString status() const;
    QVariantList frame() const;
    QVariantList bitDefinitions() const;
    QVariantList decodedSummary() const;
    int activeSecond() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

signals:
    void runningChanged();
    void pendingChanged();
    void summerTimeChanged();
    void currentTimeChanged();
    void statusChanged();
    void frameChanged();
    void activeSecondChanged();

private:
    void beginAtSecond(const QDateTime &secondBoundary);
    void pumpAudio();
    void updateDisplayedFrame(const QDateTime &minute);
    void updateClock();
    void setStatus(const QString &status);

    std::unique_ptr<JjyAudioDevice> m_audioDevice;
    std::unique_ptr<QAudioSink> m_audioSink;
    QTimer m_startTimer;
    QTimer m_clockTimer;
    QTimer m_audioPumpTimer;
    QIODevice *m_outputDevice = nullptr;
    QByteArray m_pendingAudio;
    bool m_running = false;
    bool m_pending = false;
    bool m_summerTime = false;
    QString m_status;
    QDateTime m_scheduledStart;
    QDateTime m_displayedMinute;
    QVector<double> m_displayedFrame;
    int m_activeSecond = -1;
};
