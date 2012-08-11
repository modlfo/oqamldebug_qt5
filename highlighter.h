#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H
#include <QSyntaxHighlighter>
#include <QList>
#include <QRegExp>
#include <QTextCharFormat>


struct HighlightingRule
{
    QRegExp pattern;
    QTextCharFormat format;
};
typedef QList<HighlightingRule> HighlightingRules;


#endif

