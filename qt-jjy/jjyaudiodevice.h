#pragma once

#include <QDateTime>
#include <QIODevice>
#include <QVector>

enum class JjyWaveformMode {
    LegacySquare,
    TimeStationSine,
};

class JjyAudioDevice final : public QIODevice
{
public:
    explicit JjyAudioDevice(QObject *parent = nullptr);

    // initialSecond selects where in the current JJY minute the stream starts.
    void configure(const QDateTime &firstMinute, bool summerTime, int initialSecond,
                   int sampleRate = 48000,
                   JjyWaveformMode waveformMode = JjyWaveformMode::LegacySquare);
    QVector<double> currentFrame() const;

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

private:
    void updateFrameForSample(qint64 sampleIndex);

    QDateTime m_firstMinute;
    QVector<double> m_frame;
    bool m_summerTime = false;
    int m_sampleRate = 48000;
    qint64 m_sampleIndex = 0;
    qint64 m_frameNumber = -1;
    JjyWaveformMode m_waveformMode = JjyWaveformMode::LegacySquare;
    double m_gain = 0.0;
};
