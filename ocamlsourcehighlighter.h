#ifndef OCAML_HIGHLIGHTER_H
#define OCAML_HIGHLIGHTER_H

#include "highlighter.h"
#include <QSyntaxHighlighter>
#include <QHash>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

class OCamlSourceHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    OCamlSourceHighlighter(QTextDocument *parent = 0);

    static HighlightingRules rules( const QString & );
public slots:
    void searchWord( const QString & );
protected:
    void highlightBlock(const QString &text);
    

private:
    HighlightingRules highlightingRules;

    QRegExp commentStartExpression;
    QRegExp commentEndExpression;
    QTextCharFormat multiLineCommentFormat;
};

#endif
