#include "textdiff.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <QList>
#include <QHash>
#include <QString>
#include <QTextFragment>
#include <QTextDocument>
#include <QVector>
#include <QtDebug>
#include <stdlib.h>

class TextDiff
{
    public:
        enum operation_t
        {
            DELETED,
            INSERTED,
            SAME
        };
    private:
        QString _text;
        operation_t _operation;
        int _lg;
        int _column_offset,_line_offset;
        int _start_line,_start_column,_start_pos;
        void setText(const QString &t) { _text=t; }
        void setOperation(operation_t o) { _operation=o; }
        void setLength(int lg) { _lg=lg; }
    public:
        inline TextDiff(operation_t o,const QString &t) ;
        const QString & text() const { return _text; }
        operation_t operation() const { return _operation; }
        int length() const { return _lg; }
        int startPos() const { return _start_pos; }
        int endPos() const { return _start_pos+length(); }
        int startLine() const { return _start_line; }
        int endLine() const { return _start_line+lineHeight(); }
        int startColumn() const { return _start_column; }
        int endColumn() const 
        {
            if (lineHeight() == 0) 
                return _start_column+columnWidth();
            else
                return columnWidth();
        }
        int lineHeight() const { ;return _line_offset; }
        int columnWidth() const { return _column_offset; }

        inline void mergeWith(const TextDiff &d) ;
        inline void setStartPos( int pos ) ;
        inline void setStartLine( int line ) ;
        inline void setStartColumn( int column ) ;
};


enum LCSMarker 
{
    ARROW_UP      = 0,
    ARROW_LEFT    = 1,
    ARROW_UP_LEFT = 2,
    FINAL         = 3
} ;

class LCSMatrix
{
    public :
        LCSMatrix(unsigned int mX,unsigned int mY)
        {
            _nX=mX;
            _nY=mY;
            _allocated=(mX+1);
            _allocated=_allocated*(mY+1);
            _allocated=_allocated/4;
            _allocated=_allocated+1;
            if (_allocated>=0x100000000ULL)
                _values=NULL;
            else
                _values=(unsigned char*)malloc(_allocated);
        }
        ~LCSMatrix()
        {
            free(_values);
        }

        bool allocated() const { return _values!=NULL; }

        LCSMarker get(unsigned int x,unsigned int y) const 
        {
            unsigned long long pos=_nY*x+y; 
            unsigned int shift=(pos&0x3)<<1 ;
            unsigned int table_pos=(pos>>2);
            return (LCSMarker)( ( _values[table_pos] >> shift ) & 3 );
        }
        void set(unsigned int x,unsigned int y, LCSMarker v)
        {
            unsigned long long pos=_nY*x+y; 
            unsigned int shift=(pos&0x3)<<1 ;
            unsigned int table_pos=(pos>>2);
            _values[table_pos] &= ~(3 << shift);
            _values[table_pos] |= v << shift; 
        }
    private:

        unsigned char *_values;
        unsigned int _nX,_nY;
        unsigned long long _allocated;
};

class LCSItem
{
    private:
        int _id;
        bool _is_space;
    public:
        void setIndex(int i) { _id=i; _is_space=false;}
        void setHandleAsSpace() { _is_space=true; }
        bool handleAsSpace() const { return _is_space; }
        int index() const { return _id; }
        bool operator== ( const LCSItem & x ) const
        {
            return _id==x._id;
        }
} ;

TextDiff::TextDiff(operation_t o,const QString &t) 
{
    setStartPos(0);
    setStartLine(0);
    setStartColumn(0);
    setOperation(o);
    setLength(t.length());
    _line_offset = t.count('\n');
    if (_line_offset != 0)
    {
        int last_return_pos = t.lastIndexOf('\n');
        _column_offset = length() - last_return_pos - 1;
    }
    else
        _column_offset = length() ;

    if (o==DELETED)
        setText(t); // do not record text with is present in the reference file.
}

void TextDiff::mergeWith(const TextDiff &d) 
{
    setText(text()+d.text());
    setLength(length()+d.length());
    if (d._line_offset == 0)
        _column_offset += d._column_offset;
    else
    {
        _column_offset  = d._column_offset;
        _line_offset   += d._line_offset;
    }
}

void TextDiff::setStartPos( int pos )       
{
    _start_pos    = pos; 
}

void TextDiff::setStartLine( int line )
{
    _start_line   = line;
}

void TextDiff::setStartColumn( int column ) 
{
    _start_column = column; 
}

bool wordSeparator(QChar c1,QChar c2)
{
    static QChar special[] = { '*' , '/' , '.' , '+' , '-' , '<' , '>', '!' , '=' , '~' ,'[' ,']','(',')','{','}','?',',',':',';' , '\0' } ;
    bool specialc1=false;
    bool specialc2=false;
    const QChar *sp;
    for (sp=special;*sp!='\0';sp++)
    {
        if (c1==*sp)
        {
            specialc1=true;
            break;
        }
    }
    for (sp=special;*sp!='\0';sp++)
    {
        if (c2==*sp)
        {
            specialc2=true;
            break;
        }
    }
    return specialc1!=specialc2;
}

bool onlySpaces(const QString &_text)
{
    for (QString::const_iterator it=_text.begin();it!=_text.end();++it)
    {
        if (! (*it).isSpace() )
            return false;
    }
    return true;
}

static void calcLCS(
        const QVector<LCSItem>  &X,  
        const QVector<LCSItem> &Y,  
        LCSMatrix &b   
        )
{
    int     i, j, mX, nY;
    mX = X.size()+1;
    nY = Y.size()+1;

    QVector<int> minic[2];
    minic[0].resize(nY);
    minic[1].resize(nY);

    b.set(0,0, FINAL);
    for (i=1; i<mX; i++)
    {
        b.set(i,0, ARROW_UP);
    }

    for (j=1; j<nY; j++)
    {
        minic[0][j]=0;
        b.set(0,j, ARROW_LEFT);
    }
    minic[1][0]=0;
    minic[0][0]=0;

    for (i=1; i<mX; i++)
    {
        int icur=i%2;
        int ilast=(i-1)%2;
        for (j=1; j<nY; j++)
        {
            int c;
            if (X.at(i-1) == Y.at(j-1)) 
            {
                c=minic[ilast][j-1] + 1;
                b.set(i,j,  ARROW_UP_LEFT);
            }
            else if (minic[ilast][j] > minic[icur][j-1])
            {
                c=minic[ilast][j];
                b.set(i,j,ARROW_UP);
            }
            else
            {
                c=minic[icur][j-1];
                b.set(i,j, ARROW_LEFT);
            }
            minic[icur][j]=c;
        }
    }
}



static QList<TextDiff> generateDiffList(const QVector<QString> &string_table,const QVector<LCSItem> &Xstripped, const QVector<LCSItem> &Ystripped, const QVector<LCSItem> &X, const QVector<LCSItem> &Y, const LCSMatrix &b)
{
    QList<TextDiff> diff_list;
    int i=X.size()-1;
    int j=Y.size()-1;
    int istripped=Xstripped.size()-1;
    int jstripped=Ystripped.size()-1;
    TextDiff::operation_t last_operation=TextDiff::SAME;
    bool finish=false;
    while (!finish)
    {
        LCSMarker depl=b.get(istripped+1,jstripped+1);
        switch (depl)
        {
            case ARROW_UP_LEFT:
                if (jstripped>=0)
                {
                    while (Ystripped[jstripped].index() != Y[j].index())
                    {
                        if (!string_table.at(Y[j].index()).isEmpty())
                            diff_list.prepend(TextDiff(TextDiff::SAME , string_table.at(Y[j].index()) ));
                        j--;
                    }
                    diff_list.prepend(TextDiff( TextDiff::SAME , string_table.at(Ystripped[jstripped].index()) ) );
                }
                if (istripped>=0)
                {
                    while (Xstripped[istripped].index() != X[i].index())
                    {
                        i--;
                    }
                }
                istripped--;
                jstripped--;
                i--;
                j--;
                last_operation=TextDiff::SAME;
                break;
            case ARROW_UP:
                if (istripped>=0)
                {
                    while (Xstripped[istripped].index() != X[i].index())
                    {
                        if (!string_table.at(X[i].index()).isEmpty())
                        {
                            if (last_operation==TextDiff::DELETED)
                                diff_list.prepend(TextDiff( TextDiff::DELETED , string_table.at(X[i].index()) ));
                        }
                        i--;
                    }
                    diff_list.prepend( TextDiff( TextDiff::DELETED , string_table.at(Xstripped[istripped].index()) ) );
                }
                istripped--;
                i--;
                last_operation=TextDiff::DELETED;
                break;
            case ARROW_LEFT:
                if (jstripped>=0)
                {
                    while (Ystripped[jstripped].index() != Y[j].index())
                    {
                        if (!string_table.at(Y[j].index()).isEmpty())
                        {
                            TextDiff::operation_t op = TextDiff::SAME;
                            if (last_operation==TextDiff::INSERTED)
                                op = (TextDiff::INSERTED);
                            diff_list.prepend( TextDiff( op, string_table.at(Y[j].index())) );
                        }
                        j--;
                    }
                    diff_list.prepend( TextDiff (TextDiff::INSERTED , string_table.at(Ystripped[jstripped].index()) ) );
                }
                jstripped--;
                j--;
                last_operation=TextDiff::INSERTED;
                break;
            default:
            case FINAL:
                finish=true;
                break;
        }
    }

    while (j>=0)
    {
        if (!string_table.at(Y[j].index()).isEmpty())
            diff_list.prepend(TextDiff (TextDiff::SAME , string_table.at(Y[j].index())) );
        j--;
    }
    return diff_list;
}


static void splitPolicyWord(
        QVector<LCSItem> &X,
        QVector<LCSItem> &Y,
        QVector<QString> & string_vector,
        const QString &str1,
        const QString &str2
        )
{
    int lg1=str1.length();
    int lg2=str2.length();

    X.resize(lg1+1+2);
    Y.resize(lg2+1+2);
    string_vector.resize(lg1+lg2+2);

    QHash<QString,int> string_table;
    int index=0;

    for (int istr=0;istr<2;istr++)
    {
        QVector<LCSItem> *T;
        const QString *str;
        int lg;
        if (istr==1)
        {
            T=&Y;
            str=&str2;
            lg=lg2;
        }
        else
        {
            T=&X;
            str=&str1;
            lg=lg1;
        }
        int istring=0;
        QString extracted;
        QChar last_char='\0';
        bool in_string=false;
        for (int i=0;i<lg;i++)
        {
            QChar c=str->at(i);
            bool record=false;
            if (last_char!='\\' && c=='"')
                in_string=!in_string;

            if (last_char.isSpace() == c.isSpace())
            {
                if (c.isSpace())
                    record=false;
                else if (wordSeparator(last_char,c))
                    record=true;
            }
            else
                record=true;

            if (record)
            {
                int id=string_table.value(extracted,index);
                if (id==index)
                {
                    string_vector[index]=extracted;
                    string_table[extracted]=index++;
                }
                (*T)[istring].setIndex(id);
                if (onlySpaces(extracted))
                {
                    if (!in_string)
                        (*T)[istring].setHandleAsSpace();
                }
                istring++;
                extracted.clear();
            }

            extracted+=c;
            last_char=c;
        }
        if (!extracted.isEmpty())
        {
            int id=string_table.value(extracted,index);
            if (id==index)
            {
                string_vector[index]=extracted;
                string_table[extracted]=index++;
            }
            (*T)[istring].setIndex(id);
            if (onlySpaces(extracted))
            {
                if (!in_string)
                    (*T)[istring].setHandleAsSpace();
            }
            istring++;
        }
        T->resize(istring);
    }
}



void calcDiff(QList<TextDiff> &diff,const QString &str1,const QString &str2)
{
    QVector<LCSItem> X;
    QVector<LCSItem> Y;
    QVector<LCSItem> Xstripped;
    QVector<LCSItem> Ystripped;
    QVector<QString> string_vector;
    diff.clear();
    splitPolicyWord(X,Y,string_vector,str1,str2);

    int i;
    // Start handling
    QVector<LCSItem> Start;
    int lg=qMin(X.size(),Y.size());
    Start.resize(lg);
    for (i=0;i<lg;i++)
    {
        if (X[i]==Y[i])
            Start[i]=X[i];
        else
            break;
    }
    Start.resize(i);
    if (i>0)
    {
        X.remove(0,i);
        Y.remove(0,i);
    }

    // End handling
    int Xlg=X.size();
    QVector<LCSItem> End;
    int Ylg=Y.size();
    lg=qMin(Xlg,Ylg);
    Ylg--; Xlg--;
    End.resize(lg);
    for (i=0;i<lg;i++)
    {
        if (X[Xlg-i]==Y[Ylg-i])
            End[i]=X[Xlg-i];
        else
            break;
    }
    int EndLg=i;
    End.resize(EndLg);
    if (EndLg>0)
    {
        X.remove(X.size()-EndLg,EndLg);
        Y.remove(Y.size()-EndLg,EndLg);
    }

    QVector<LCSItem>::const_iterator it;
    for (it=X.begin();it!=X.end();++it)
    {
        if ( ! (*it).handleAsSpace() )
            Xstripped.append(*it);
    }
    for (it=Y.begin();it!=Y.end();++it)
    {
        if ( ! (*it).handleAsSpace() )
            Ystripped.append(*it);
    }

    int lgX=Xstripped.size();
    int lgY=Ystripped.size();

    diff.clear();
    LCSMatrix *b = new LCSMatrix (lgX+1,lgY+1);
    if (b->allocated())
    {
        calcLCS( Xstripped, Ystripped, *b ); 
        QList<TextDiff> diff_list=generateDiffList(string_vector,Xstripped,Ystripped,X,Y,*b);
        delete b;
        b=NULL;
        for (i=Start.size()-1;i>=0;i--)
        {
            if (!string_vector.at(Start[i].index()).isEmpty())
                diff_list.prepend( TextDiff( TextDiff::SAME , string_vector.at(Start[i].index()) ) );
        }
        for (i=End.size()-1;i>=0;i--)
        {
            if (!string_vector.at(End[i].index()).isEmpty())
                diff_list.append( TextDiff (TextDiff::SAME , string_vector.at(End[i].index())) );
        }

        // Compact list
        int last_pos    = 0 ;
        int last_line   = 0 ;
        int last_column = 0 ;
        for (QList<TextDiff>::const_iterator it=diff_list.begin();it!=diff_list.end();++it)
        {
            if (diff.isEmpty())
                diff.append(*it);
            else
            {
                TextDiff &lastItem(diff.last());
                if (lastItem.operation()==(*it).operation())
                    lastItem.mergeWith( *it );
                else
                {
                    last_pos    = lastItem.startPos() + lastItem.length() ;
                    last_line   = lastItem.startLine() + lastItem.lineHeight() ;
                    if (lastItem.lineHeight() == 0)
                        last_column = lastItem.startColumn() + lastItem.columnWidth() ;
                    else
                        last_column = lastItem.columnWidth() ;

                    TextDiff item(*it);
                    item.setStartPos( last_pos );
                    item.setStartLine( last_line );
                    item.setStartColumn( last_column );
                    diff.append(item);
                }
            }
        }
    }

    if (b)
    {
        delete b;
        b=NULL;
    }
}

QString htmlDiff( const QString &cur, const QString &ref )
{
    QList<TextDiff> diff;
    calcDiff( diff, ref, cur );

    QString ret = "<HTML><BODY>";
    int pos=0;
    for (QList<TextDiff>::const_iterator it=diff.begin();it!=diff.end();++it)
    {
        switch( it->operation())
        {
            case TextDiff::DELETED:
                { 
                    QString text = it->text();
                    ret += "<span style=\"text-decoration: line-through;\"><font color=\"gray\">"; 
                    ret += text.toHtmlEscaped();
                    ret += "</font></span>"; 
                }
                break;
            case TextDiff::INSERTED:
                {
                    QString text = cur.mid( pos, it->length() );
                    ret += "<span style=\"text-decoration: underline;\"><B><font color=\"blue\">"; 
                    ret += text.toHtmlEscaped();
                    ret += "</font></B></span>"; 
                    pos += it->length();
                }
                break;
            case TextDiff::SAME: 
                { 
                    QString text = cur.mid( pos, it->length() );
                    ret += text.toHtmlEscaped();
                    pos += it->length();
                }
                break;
        }
    }
    ret += "</BODY></HTML>";
    return ret;
}


