#include <QtGui>

#include "ocamldebughighlighter.h"
#include "ocamlsourcehighlighter.h"

OCamlDebugHighlighter::OCamlDebugHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    QTextCharFormat keywordFormat;
    QTextCharFormat positionFormat;
    QTextCharFormat separatorFormat;
    QTextCharFormat keywordAbrevFormat;
    QTextCharFormat emacsFormat;
    QTextCharFormat errorFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat promptFormat;
    QTextCharFormat cursorFormat;
    QTextCharFormat dbgInfoFormat;
    HighlightingRule rule;

    highlightingRulesDebugger << OCamlSourceHighlighter::rules( QString() );

    keywordAbrevFormat.setForeground(Qt::darkMagenta);

    keywordFormat.setForeground(Qt::darkMagenta);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns 
        << "backstep"
        << "backtrace"
        << "break"
        << "bt"
        << "cd"
        << "complete"
        << "d"
        << "delete"
        << "directory"
        << "display"
        << "down"
        << "environment"
        << "finish"
        << "frame"
        << "goto"
        << "help"
        << "info"
        << "install_printer"
        << "kill"
        << "last"
        << "list"
        << "load_printer"
        << "next"
        << "p"
        << "previous"
        << "print"
        << "pwd"
        << "quit"
        << "remove_printer"
        << "reverse"
        << "run"
        << "set"
        << "shell"
        << "show"
        << "source"
        << "start"
        << "step"
        << "up"
        ;

    foreach (const QString &pattern, keywordPatterns) 
    {
        rule.pattern = QRegExp("\\b"+pattern+"\\b");
        rule.format = keywordFormat;
        highlightingRulesUser.append(rule);
        int pattern_lg = pattern.length() ;
        for (int l = 1 ; l < pattern_lg ; l++)
        {
            QString abbrev = pattern.left(l);
            if ( ! keywordPatterns.contains( abbrev ) )
            {
                rule.pattern = QRegExp("\\b"+abbrev+"\\b");
                rule.format = keywordAbrevFormat;
                highlightingRulesUser.append(rule);
            }
        }
    }

    dbgInfoFormat.setForeground(Qt::darkRed);
    QStringList dbgInfoKeywords;
    dbgInfoKeywords
        << "^#[0-9]*  *Pc : [0-9]+ .*$"
        << "Time : [0-9]+"
        << "Breakpoint : [0-9]+"
        << "Breakpoints :( [0-9]+)+"
        << " - pc : [0-9]+"
        << " - module [A-Z][a-z.A-Z0-9_]*"
        << "Program exit."
        << "No source file for [A-Z][a-z.A-Z0-9_]*\\."
        << "Breakpoint [0-9]+ at [0-9]+ : .*$"
        << "Removed breakpoint [0-9]+ at [0-9]+ : .*$"
        ;
    foreach (const QString &pattern, dbgInfoKeywords) 
    {
        rule.pattern = QRegExp(pattern);
        rule.format = dbgInfoFormat;
        highlightingRulesDebugger.append(rule);
    }

    emacsFormat.setForeground(Qt::red);
    rule.pattern = QRegExp("^\\x001A\\x001A.*$");
    rule.format = emacsFormat;
    highlightingRulesAll.append(rule);

    QStringList errorPatterns;
    errorPatterns 
        << "^Syntax error\\."
        << "^Unknown command\\."
        << "^Ambiguous command\\."
        << "^Unbound identifier .*"
        << tr("^Application .* is modified\\.$")
        << tr("^OCamlDebug process stopped\\.$")
        ;
    errorFormat.setForeground(Qt::red);
    errorFormat.setFontWeight(QFont::Bold);
    foreach (const QString &pattern, errorPatterns) 
    {
        rule.pattern = QRegExp(pattern);
        rule.format = errorFormat;
        highlightingRulesAll.append(rule);
    }

    positionFormat.setBackground(Qt::yellow);
    rule.pattern = QRegExp( "<\\|[ba]\\|>");
    rule.format = positionFormat;
    highlightingRulesDebugger.append(rule);

    cursorFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp("\\x2592");
    rule.format = cursorFormat;
    highlightingRulesUser.append(rule);

    promptFormat.setBackground(Qt::cyan);
    promptFormat.setForeground(Qt::darkBlue);
    rule.pattern = QRegExp("\\(ocd\\)");
    rule.format = promptFormat;
    highlightingRulesUser.append(rule);

    quotationFormat.setForeground(Qt::darkRed);
    rule.pattern = QRegExp("[A-Z][a-z._A-Z0-9]*\\(\".*\"\\)");
    rule.format = quotationFormat;
    highlightingRulesDebugger.append(rule);
}

void OCamlDebugHighlighter::highlightBlock(const QString &text)
{
    QRegExp user_input_rx("^\\(ocd\\).*$");
    QList< QList<HighlightingRule> > highlightingRules ;
     highlightingRules << highlightingRulesAll;
    if ( user_input_rx.exactMatch(text))
        highlightingRules << highlightingRulesUser;
    else
        highlightingRules << highlightingRulesDebugger;

    foreach (const QList<HighlightingRule> &rules, highlightingRules) 
        foreach (const HighlightingRule &rule, rules) 
        {
            QRegExp expression(rule.pattern);
            int index = expression.indexIn(text);
            while (index >= 0) 
            {
                int length = expression.matchedLength();
                setFormat(index, length, rule.format);
                index = expression.indexIn(text, index + length);
            }
        }
    setCurrentBlockState(0);
}
