#include "jjyframebuilder.h"

#include <QDate>

QVector<double> JjyFrameBuilder::build(const QDateTime &localMinute, bool summerTime)
{
    QVector<double> frame;
    frame.reserve(60);

    auto marker = [&frame]() { frame.append(0.2); };
    int parity = 0;
    auto bit = [&frame, &parity](int &value, int weight) {
        const bool one = value >= weight;
        if (one) {
            value -= weight;
            ++parity;
        }
        frame.append(one ? 0.5 : 0.8);
    };
    auto fixedBit = [&frame](bool one) { frame.append(one ? 0.5 : 0.8); };

    const QDate date = localMinute.date();
    int minute = localMinute.time().minute();
    int hour = localMinute.time().hour();
    int dayOfYear = date.dayOfYear();
    int year = date.year() % 100;
    int weekday = date.dayOfWeek() % 7; // JJY: Sunday=0, Monday=1, ... Saturday=6

    marker();

    parity = 0;
    for (int weight : {40, 20, 10}) bit(minute, weight);
    fixedBit(false); // :04 unused
    for (int weight : {8, 4, 2, 1}) bit(minute, weight);
    const int minuteParity = parity;
    marker();

    parity = 0;
    fixedBit(false); // :10 unused
    fixedBit(false); // :11 unused
    for (int weight : {20, 10}) bit(hour, weight);
    fixedBit(false); // :14 unused
    for (int weight : {8, 4, 2, 1}) bit(hour, weight);
    const int hourParity = parity;
    marker();

    fixedBit(false); // :20 unused
    fixedBit(false); // :21 unused
    for (int weight : {200, 100}) bit(dayOfYear, weight);
    fixedBit(false); // :24 unused
    for (int weight : {80, 40, 20, 10}) bit(dayOfYear, weight);
    marker();

    for (int weight : {8, 4, 2, 1}) bit(dayOfYear, weight);
    fixedBit(false);
    fixedBit(false);
    fixedBit(hourParity % 2 != 0);
    fixedBit(minuteParity % 2 != 0);
    fixedBit(false); // SU1
    marker();

    fixedBit(summerTime); // SU2
    for (int weight : {80, 40, 20, 10, 8, 4, 2, 1}) bit(year, weight);
    marker();

    for (int weight : {4, 2, 1}) bit(weekday, weight);
    fixedBit(false); // leap-second announcement: not implemented
    fixedBit(false);
    fixedBit(false);
    fixedBit(false);
    fixedBit(false);
    fixedBit(false);
    marker();

    Q_ASSERT(frame.size() == 60);
    return frame;
}
