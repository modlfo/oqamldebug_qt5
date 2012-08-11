#ifndef FILE_SYSTEM_WATCHER_H
#define FILE_SYSTEM_WATCHER_H
#include <QFileSystemWatcher>
#include <QString>

class FileSystemWatcher : public QFileSystemWatcher
{
  Q_OBJECT

  public:
    FileSystemWatcher ( const QString& file);

    virtual ~FileSystemWatcher();

  private:
    void watchFile(const QString &);
    QString _file;

  private slots:
    void directoryChangedSlot ( const QString & );
    void fileChangedSlot ( const QString & );
  signals:
    void fileChanged();
};

#endif
