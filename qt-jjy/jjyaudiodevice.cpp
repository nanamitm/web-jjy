#include "jjyaudiodevice.h"

#include "jjyframebuilder.h"

#include <QtMath>
#include <algorithm>
#include <cstring>

namespace {
constexpr double CarrierHz = 13333.3333333333;
constexpr qint16 Amplitude = 26000;
constexpr double LowGain = 0.31622776; // -10 dB
constexpr double GainLerpRate = 0.015;
}

JjyAudioDevice::JjyAudioDevice(QObject *parent) : QIODevice(parent)
{
}

void JjyAudioDevice::configure(const QDateTime &firstMinute, bool summerTime, int initialSecond,
                               int sampleRate, JjyWaveformMode waveformMode)
{
    Q_ASSERT(!isOpen());
    m_firstMinute = firstMinute;
    m_summerTime = summerTime;
    m_sampleRate = sampleRate;
    m_sampleIndex = qBound(0, initialSecond, 59) * qint64(m_sampleRate);
    m_frameNumber = -1;
    m_frame.clear();
    m_waveformMode = waveformMode;
    m_gain = waveformMode == JjyWaveformMode::TimeStationSine ? LowGain : 0.0;
}

QVector<double> JjyAudioDevice::currentFrame() const
{
    return m_frame;
}

qint64 JjyAudioDevice::readData(char *data, qint64 maxSize)
{
    const qint64 sampleCount = maxSize / qint64(sizeof(qint16));
    auto *output = reinterpret_cast<qint16 *>(data);

    for (qint64 i = 0; i < sampleCount; ++i, ++m_sampleIndex) {
        updateFrameForSample(m_sampleIndex);
        const qint64 inMinute = m_sampleIndex % (qint64(m_sampleRate) * 60);
        const int second = int(inMinute / m_sampleRate);
        const int sampleInSecond = int(inMinute % m_sampleRate);
        const int pulseSamples = qRound(m_frame.at(second) * m_sampleRate);

        // DDS keeps the 13.333 kHz carrier phase continuous across pulse edges.
        const double phase = (2.0 * M_PI * CarrierHz * double(m_sampleIndex)) / m_sampleRate;
        const double sine = qSin(phase);

        if (m_waveformMode == JjyWaveformMode::LegacySquare) {
            output[i] = sampleInSecond < pulseSamples ?
                            (sine >= 0.0 ? Amplitude : -Amplitude) :
                            0;
            continue;
        }

        // Time Station style: send a continuous sine carrier, lowering it by
        // 10 dB outside each 0.2 / 0.5 / 0.8 second high-amplitude interval.
        const double targetGain = sampleInSecond < pulseSamples ? 1.0 : LowGain;
        m_gain += (targetGain - m_gain) * GainLerpRate;
        output[i] = qRound(sine * Amplitude * m_gain);
    }

    const qint64 bytesWritten = sampleCount * qint64(sizeof(qint16));
    if (bytesWritten < maxSize)
        std::memset(data + bytesWritten, 0, size_t(maxSize - bytesWritten));
    return maxSize;
}

qint64 JjyAudioDevice::writeData(const char *, qint64)
{
    return -1;
}

void JjyAudioDevice::updateFrameForSample(qint64 sampleIndex)
{
    const qint64 frameNumber = sampleIndex / (qint64(m_sampleRate) * 60);
    if (frameNumber == m_frameNumber)
        return;

    m_frameNumber = frameNumber;
    m_frame = JjyFrameBuilder::build(m_firstMinute.addSecs(frameNumber * 60), m_summerTime);
}
