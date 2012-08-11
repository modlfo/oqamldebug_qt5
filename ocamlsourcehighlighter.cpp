#include <QtGui>

#include "ocamlsourcehighlighter.h"

OCamlSourceHighlighter::OCamlSourceHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    commentStartExpression = QRegExp("\\(\\*");
    commentEndExpression = QRegExp("\\*\\)");
    multiLineCommentFormat.setForeground(Qt::gray);


    highlightingRules = rules();
}

HighlightingRules OCamlSourceHighlighter::rules()
{
    QTextCharFormat keywordFormat;
    QTextCharFormat operatorFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    HighlightingRules highlightingRules;
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;

    keywordPatterns 
        << "and"
        << "as"
        << "assert"
        << "begin"
        << "class"
        << "constraint" 
        << "do"
        << "done"
        << "downto"
        << "else"
        << "end" 
        << "exception"
        << "exception" 
        << "external" 
        << "false" 
        << "for"
        << "for" 
        << "fun"
        << "function"
        << "functor"
        << "if"
        << "in"
        << "include"
        << "inherit"
        << "initializer"
        << "lazy"
        << "let"
        << "match"
        << "method!"
        << "method"
        << "module"
        << "mutable"
        << "new"
        << "object"
        << "of"
        << "open"
        << "or"
        << "private"
        << "rec"
        << "sig" 
        << "struct"
        << "then"
        << "to"
        << "true"
        << "try"
        << "type"
        << "val" 
        << "virtual"
        << "when"
        << "while"
        << "with"
        ;


    foreach (const QString &pattern, keywordPatterns)
    {
        rule.pattern = QRegExp("\\b"+pattern+"\\b");
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    operatorFormat.setForeground(Qt::darkCyan);
    operatorFormat.setFontWeight(QFont::Bold);
    QStringList operatorPattern;
    operatorPattern 
        << ":" 
        << "<" 
        << ">" 
        << "^" 
        << ";" 
        << "!" 
        << "|" 
        << "." 
        << "#" 
        << "->" 
        << "=" 
        ;

    foreach (const QString &pattern, operatorPattern)
    {
        rule.pattern = QRegExp( QRegExp::escape( pattern) );
        rule.format = operatorFormat;
        highlightingRules.append(rule);
    }


    QStringList quotationPattern;
    quotationPattern 
        << "\".*\""
        << "\\b0x[0-9a-fa-F]+\\b"
        << "\\b0o[0-7]+\\b"
        << "\\b[0-9]+\\b"
        ;
    quotationFormat.setForeground(Qt::darkGreen);
    foreach (const QString &pattern, quotationPattern)
    {
        rule.pattern = QRegExp(pattern);
        rule.format = quotationFormat;
        highlightingRules.append(rule);
    }

    rule.pattern = QRegExp("0x[0-9a-fa-F]+");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    return highlightingRules;
}

void OCamlSourceHighlighter::highlightBlock( const QString &text )
{
    foreach ( const HighlightingRule & rule, highlightingRules )
    {
        QRegExp expression( rule.pattern );
        int index = expression.indexIn( text );
        while ( index >= 0 )
        {
            int length = expression.matchedLength();
            setFormat( index, length, rule.format );
            index = expression.indexIn( text, index + length );
        }
    }
    setCurrentBlockState( 0 );

    int startIndex = 0;
    if ( previousBlockState() != 1 )
        startIndex = commentStartExpression.indexIn( text );

    while ( startIndex >= 0 )
    {
        int endIndex = commentEndExpression.indexIn( text, startIndex );
        int commentLength;
        if ( endIndex == -1 )
        {
            setCurrentBlockState( 1 );
            commentLength = text.length() - startIndex;
        }
        else
        {
            commentLength = endIndex - startIndex
                + commentEndExpression.matchedLength();
        }
        setFormat( startIndex, commentLength, multiLineCommentFormat );
        startIndex = commentStartExpression.indexIn( text, startIndex + commentLength );
    }
}
