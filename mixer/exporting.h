#ifndef EXPORTING_H
#define EXPORTING_H

#include "soundprocessing.h"

//used to export recording to file
class Exporting : public QObject
{
    Q_OBJECT
private:
     SoundProcessing* soundProc;

public:
     explicit Exporting(SoundProcessing* soundprocessing, QObject *parent = nullptr);
     //prepare wave header
     void writeWavHeader( QFile * file );
     //fill missing data, file size etc.
     void closeWavHeader( QFile * file );

signals:
     void exportReady();

public slots:
     void exportFile(QString filename);

};

#endif // EXPORTING_H
