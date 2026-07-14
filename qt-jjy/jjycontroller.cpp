#include "jjycontroller.h"

#include "jjyframebuilder.h"

#include <QAudioDevice>
#include <QDateTime>
#include <QMediaDevices>
#include <QVariant>

namespace {
int weightedBitCount(int value, std::initializer_list<int> weights)
{
    int count = 0;
    for (const int weight : weights) {
        if (value >= weight) {
            value -= weight;
            ++count;
        }
    }
    return count;
}
}

JjyController::JjyController(QObject *parent) : QObject(parent)
{
    m_startTimer.setSingleShot(true);
    connect(&m_startTimer, &QTimer::timeout, this, [this] { beginAtSecond(m_scheduledStart); });

    m_clockTimer.setInterval(250);
    connect(&m_clockTimer, &QTimer::timeout, this, &JjyController::updateClock);
    m_clockTimer.start();

    m_audioPumpTimer.setInterval(5);
    connect(&m_audioPumpTimer, &QTimer::timeout, this, &JjyController::pumpAudio);
    updateClock();
    const QDateTime now = QDateTime::currentDateTime();
    updateDisplayedFrame(now.addSecs(-now.time().second()).addMSecs(-now.time().msec()));
    setStatus(tr("停止中"));
}

JjyController::~JjyController()
{
    stop();
}

bool JjyController::running() const { return m_running; }
bool JjyController::pending() const { return m_pending; }
bool JjyController::summerTime() const { return m_summerTime; }

void JjyController::setSummerTime(bool enabled)
{
    if (m_summerTime == enabled)
        return;
    m_summerTime = enabled;
    m_displayedFrame.clear();
    updateDisplayedFrame(m_displayedMinute);
    emit summerTimeChanged();
}

QString JjyController::currentTime() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz t");
}

QString JjyController::status() const { return m_status; }

QVariantList JjyController::frame() const
{
    QVariantList values;
    for (double value : m_displayedFrame)
        values.append(value);
    return values;
}

QVariantList JjyController::bitDefinitions() const
{
    struct Definition {
        const char *name;
        const char *weight;
        const char *description;
        const char *category;
        const char *field;
    };
    static const Definition definitions[60] = {
        {"M", "", "分の開始マーカー", "marker", ""},
        {"分", "40", "分（10分台）", "minute", "minute"}, {"分", "20", "分（10分台）", "minute", "minute"},
        {"分", "10", "分（10分台）", "minute", "minute"}, {"未使用", "0", "常に0", "unused", ""},
        {"分", "8", "分（1分台）", "minute", "minute"}, {"分", "4", "分（1分台）", "minute", "minute"},
        {"分", "2", "分（1分台）", "minute", "minute"}, {"分", "1", "分（1分台）", "minute", "minute"},
        {"P1", "", "ポジションマーカー", "marker", ""},
        {"未使用", "0", "常に0", "unused", ""}, {"未使用", "0", "常に0", "unused", ""},
        {"時", "20", "時（10時台）", "hour", "hour"}, {"時", "10", "時（10時台）", "hour", "hour"},
        {"未使用", "0", "常に0", "unused", ""},
        {"時", "8", "時（1時台）", "hour", "hour"}, {"時", "4", "時（1時台）", "hour", "hour"},
        {"時", "2", "時（1時台）", "hour", "hour"}, {"時", "1", "時（1時台）", "hour", "hour"},
        {"P2", "", "ポジションマーカー", "marker", ""},
        {"未使用", "0", "常に0", "unused", ""}, {"未使用", "0", "常に0", "unused", ""},
        {"通算日", "200", "1月1日=1", "day", "day"}, {"通算日", "100", "1月1日=1", "day", "day"},
        {"未使用", "0", "常に0", "unused", ""},
        {"通算日", "80", "1月1日=1", "day", "day"}, {"通算日", "40", "1月1日=1", "day", "day"},
        {"通算日", "20", "1月1日=1", "day", "day"}, {"通算日", "10", "1月1日=1", "day", "day"},
        {"P3", "", "ポジションマーカー", "marker", ""},
        {"通算日", "8", "1月1日=1", "day", "day"}, {"通算日", "4", "1月1日=1", "day", "day"},
        {"通算日", "2", "1月1日=1", "day", "day"}, {"通算日", "1", "1月1日=1", "day", "day"},
        {"未使用", "0", "常に0", "unused", ""}, {"未使用", "0", "常に0", "unused", ""},
        {"PA1", "", "時の偶数パリティ", "parity", "parity"}, {"PA2", "", "分の偶数パリティ", "parity", "parity"},
        {"SU1", "", "夏時間変更予告（現行では0）", "control", ""},
        {"P4", "", "ポジションマーカー", "marker", ""}, {"SU2", "", "夏時間実施中なら1", "control", ""},
        {"年", "80", "西暦下2桁（10年台）", "year", "year"}, {"年", "40", "西暦下2桁（10年台）", "year", "year"},
        {"年", "20", "西暦下2桁（10年台）", "year", "year"}, {"年", "10", "西暦下2桁（10年台）", "year", "year"},
        {"年", "8", "西暦下2桁（1年台）", "year", "year"}, {"年", "4", "西暦下2桁（1年台）", "year", "year"},
        {"年", "2", "西暦下2桁（1年台）", "year", "year"}, {"年", "1", "西暦下2桁（1年台）", "year", "year"},
        {"P5", "", "ポジションマーカー", "marker", ""},
        {"曜日", "4", "日曜=0、土曜=6", "weekday", "weekday"}, {"曜日", "2", "日曜=0、土曜=6", "weekday", "weekday"},
        {"曜日", "1", "日曜=0、土曜=6", "weekday", "weekday"},
        {"LS1", "", "うるう秒実施月なら1", "control", ""}, {"LS2", "", "うるう秒：挿入=1、削除=0", "control", ""},
        {"未使用", "0", "常に0", "unused", ""}, {"未使用", "0", "常に0", "unused", ""},
        {"未使用", "0", "常に0", "unused", ""}, {"未使用", "0", "常に0", "unused", ""},
        {"P0", "", "分の終了マーカー", "marker", ""},
    };

    QVariantList result;
    result.reserve(60);
    for (int second = 0; second < 60; ++second) {
        const auto &definition = definitions[second];
        result.append(QVariantMap{{"second", second}, {"name", definition.name},
                                  {"weight", definition.weight}, {"description", definition.description},
                                  {"category", definition.category}, {"field", definition.field}});
    }
    return result;
}

QVariantList JjyController::decodedSummary() const
{
    if (!m_displayedMinute.isValid())
        return {};

    const QDate date = m_displayedMinute.date();
    const int minute = m_displayedMinute.time().minute();
    const int hour = m_displayedMinute.time().hour();
    const int day = date.dayOfYear();
    const int year = date.year() % 100;
    const int weekday = date.dayOfWeek() % 7;
    const QStringList weekdayNames{tr("日"), tr("月"), tr("火"), tr("水"), tr("木"), tr("金"), tr("土")};

    const int hourParity = weightedBitCount(hour, {20, 10, 8, 4, 2, 1}) % 2;
    const int minuteParity = weightedBitCount(minute, {40, 20, 10, 8, 4, 2, 1}) % 2;
    auto item = [](const QString &label, const QString &value, const QString &field,
                   int second, const QString &category) {
        return QVariantMap{{"label", label}, {"value", value},
                           {"field", field}, {"second", second}, {"category", category}};
    };

    return {
        item(tr("分"), QString::number(minute), "minute", 1, "minute"),
        item(tr("時"), QString::number(hour), "hour", 12, "hour"),
        item(tr("通算日"), QString::number(day), "day", 22, "day"),
        item(tr("年"), QString::number(year), "year", 41, "year"),
        item(tr("曜日"), weekdayNames.at(weekday), "weekday", 50, "weekday"),
        item(tr("パリティ"), QStringLiteral("PA1=%1 / PA2=%2").arg(hourParity).arg(minuteParity), "parity", 36, "parity"),
    };
}

int JjyController::activeSecond() const { return m_activeSecond; }

void JjyController::start()
{
    if (m_running || m_startTimer.isActive())
        return;

    const QDateTime now = QDateTime::currentDateTime();
    // Begin on the next whole second so a receiver can acquire signal before
    // the following minute boundary.  The audio generator starts at this
    // second's position within the current JJY minute.
    m_scheduledStart = now.addSecs(1).addMSecs(-now.time().msec());
    const qint64 delay = qMax<qint64>(0, now.msecsTo(m_scheduledStart));
    const QDateTime scheduledMinute = m_scheduledStart.addSecs(-m_scheduledStart.time().second());
    updateDisplayedFrame(scheduledMinute);
    m_pending = true;
    emit pendingChanged();
    setStatus(tr("%1 から送信を開始します").arg(m_scheduledStart.toString("hh:mm:ss")));
    m_startTimer.start(int(delay));
}

void JjyController::stop()
{
    m_startTimer.stop();
    m_audioPumpTimer.stop();
    m_outputDevice = nullptr;
    m_pendingAudio.clear();
    if (m_pending) {
        m_pending = false;
        emit pendingChanged();
    }
    if (m_audioSink)
        m_audioSink->stop();
    if (m_audioDevice && m_audioDevice->isOpen())
        m_audioDevice->close();
    m_audioSink.reset();
    m_audioDevice.reset();
    if (m_running) {
        m_running = false;
        emit runningChanged();
    }
    setStatus(tr("停止中"));
    emit frameChanged();
}

void JjyController::beginAtSecond(const QDateTime &secondBoundary)
{
    if (m_pending) {
        m_pending = false;
        emit pendingChanged();
    }
    const int initialSecond = secondBoundary.time().second();
    const QDateTime minute = secondBoundary.addSecs(-initialSecond);
    updateDisplayedFrame(minute);
    const QAudioDevice output = QMediaDevices::defaultAudioOutput();
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    if (!output.isFormatSupported(format))
        format = output.preferredFormat();

    // The generator currently emits signed 16-bit mono PCM; abort rather than
    // silently transmitting a differently interpreted sample format.
    if (format.sampleFormat() != QAudioFormat::Int16 || format.channelCount() != 1) {
        setStatus(tr("この出力デバイスは16-bitモノラルPCMをサポートしていません"));
        return;
    }

    m_audioDevice = std::make_unique<JjyAudioDevice>();
    m_audioDevice->configure(minute, m_summerTime, initialSecond, format.sampleRate());
    m_audioDevice->open(QIODevice::ReadOnly);
    m_audioSink = std::make_unique<QAudioSink>(output, format);
    m_audioSink->setBufferSize(format.bytesForDuration(200'000));
    m_audioSink->setVolume(1.0);
    m_outputDevice = m_audioSink->start();

    if (!m_outputDevice || m_audioSink->error() != QtAudio::NoError) {
        setStatus(tr("音声出力を開始できませんでした（エラーコード: %1）")
                      .arg(int(m_audioSink->error())));
        m_audioSink.reset();
        m_outputDevice = nullptr;
        m_audioDevice->close();
        m_audioDevice.reset();
        return;
    }

    m_running = true;
    emit runningChanged();
    pumpAudio();
    m_audioPumpTimer.start();
    setStatus(tr("送信中: %1").arg(output.description()));
}

void JjyController::pumpAudio()
{
    if (!m_audioSink || !m_audioDevice || !m_outputDevice)
        return;

    // Explicit push mode avoids depending on the platform backend to pull
    // from our custom QIODevice. Retain unwritten bytes across partial writes.
    for (int attempt = 0; attempt < 8; ++attempt) {
        if (m_pendingAudio.isEmpty()) {
            constexpr qint64 chunkSize = 4096;
            const qint64 freeBytes = m_audioSink->bytesFree();
            if (freeBytes < qint64(sizeof(qint16)))
                return;
            const qint64 size = qMin(chunkSize, freeBytes) & ~qint64(1);
            m_pendingAudio.resize(qsizetype(size));
            const qint64 bytesRead = m_audioDevice->read(m_pendingAudio.data(), size);
            if (bytesRead <= 0) {
                setStatus(tr("音声データを生成できませんでした"));
                return;
            }
            m_pendingAudio.resize(qsizetype(bytesRead));
        }

        const qint64 bytesWritten = m_outputDevice->write(m_pendingAudio);
        if (bytesWritten < 0) {
            setStatus(tr("音声データを出力できませんでした"));
            return;
        }
        if (bytesWritten == 0)
            return;
        m_pendingAudio.remove(0, qsizetype(bytesWritten));
    }
}

void JjyController::updateClock()
{
    emit currentTimeChanged();
    const QDateTime now = QDateTime::currentDateTime();
    const int second = now.time().second();
    if (m_activeSecond != second) {
        m_activeSecond = second;
        emit activeSecondChanged();
    }

    if (!m_running)
        return;
    updateDisplayedFrame(now.addSecs(-now.time().second()).addMSecs(-now.time().msec()));
}

void JjyController::updateDisplayedFrame(const QDateTime &minute)
{
    if (m_displayedMinute == minute && !m_displayedFrame.isEmpty())
        return;
    m_displayedMinute = minute;
    m_displayedFrame = JjyFrameBuilder::build(minute, m_summerTime);
    emit frameChanged();
}

void JjyController::setStatus(const QString &status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}
