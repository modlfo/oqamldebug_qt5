#include "filesystemwatcher.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>

FileSystemWatcher::FileSystemWatcher ( const QString& file  ): QFileSystemWatcher ( )
{
  connect(this,SIGNAL( fileChanged ( const QString & )),
      this,SLOT( fileChangedSlot ( const QString & )));
  connect(this,SIGNAL( directoryChanged ( const QString & )),
      this,SLOT( directoryChangedSlot ( const QString & )));
  _file=file;
  watchFile(file);
}

void FileSystemWatcher::watchFile(const QString &file)
{
  if (!files().isEmpty())
    removePaths(files());
  if (!directories().isEmpty())
    removePaths(directories());
  QFileInfo fileInfo(file);
  if (fileInfo.exists())
    addPath(file);
  QString filePath=fileInfo.absoluteDir().path();
  QFileInfo filePathInfo(filePath);
  if (filePathInfo.exists())
    addPath(filePath);
}

void FileSystemWatcher::fileChangedSlot ( const QString & )
{
  QFileInfo fileInfo(_file);
  if ( fileInfo.exists() )
    emit fileChanged();
}

void FileSystemWatcher::directoryChangedSlot ( const QString & )
{
  if (!files().contains(_file))
  {
    QFileInfo fileInfo(_file);
    if ( fileInfo.exists() )
    {
      addPath(_file);
      emit fileChanged();
    }
  }
}

FileSystemWatcher::~FileSystemWatcher()
{
}
