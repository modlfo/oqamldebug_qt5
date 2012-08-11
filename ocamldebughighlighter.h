#ifndef OCAML_DEBUG_HIGHLIGHTER_H
#define OCAML_DEBUG_HIGHLIGHTER_H

#include "highlighter.h"
#include <QSyntaxHighlighter>
#include <QHash>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

class OCamlDebugHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    OCamlDebugHighlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);

private:
    HighlightingRules highlightingRulesDebugger;
    HighlightingRules highlightingRulesUser;
    HighlightingRules highlightingRulesAll;
};

#endif
