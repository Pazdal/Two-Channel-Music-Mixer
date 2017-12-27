#ifndef SOUNDPROCESSING_H
#define SOUNDPROCESSING_H

#include <QObject>
#include <QThread>
#include <QCoreApplication>

#include <QTimer>

#include <QAudioFormat>
#include <QAudioBuffer>
#include <QAudioOutput>

#include <mixpanel.h>

class SoundProcessing : public QObject
{
    Q_OBJECT

public:
    explicit SoundProcessing(QObject *parent = nullptr);
    bool abort;

    int crossFader;

    MixPanel panel1;
    MixPanel panel2;

    double buffer1[256], buffer2[256];

private:
    QAudioOutput *audioOutput;
    QIODevice *audioDevice;

    QTimer* timer;

    double rate;

public slots:
    void play();
};

#endif // SOUNDPROCESSING_H

