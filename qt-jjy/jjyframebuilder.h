#pragma once

#include <QDateTime>
#include <QVector>

// Builds the 60 one-second JJY pulse widths for one local-time minute.
class JjyFrameBuilder
{
public:
    static QVector<double> build(const QDateTime &localMinute, bool summerTime);
};
