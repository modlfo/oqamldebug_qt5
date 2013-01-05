#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <QString>
#include <QMap>

struct BreakPoint
{
    QString command;
    int id;
    QString file;
    int fromLine, toLine, fromColumn, toColumn;
};

typedef QMap<int,BreakPoint> BreakPoints;


#endif

