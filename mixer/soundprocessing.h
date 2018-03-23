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
#include <action.h>

//class to mix audio from panels and output it
class SoundProcessing : public QObject
{
    Q_OBJECT
    Q_ENUMS(Actions)

public:
    explicit SoundProcessing(QObject *parent = nullptr); 

    //action id
    enum Actions { low=1, med, high, cross, volume, tempo };

    //panels for mixing audio/add effects
    MixPanel panel1;
    MixPanel panel2;

    //class to save/load actions
    Action action;

    //array for recorded audio
    QByteArray output1;

    //decoded audio format
    QAudioFormat format;
    QTimer* timer;

    //crossFader value
    int crossFader;

    void launchActions(quint64 actPos1, quint64 actPos2);

private:
    //audio output device
    QAudioOutput *audioOutput;
    QIODevice *audioDevice;

    //enable/disable recording of output
    bool isRecording;

    //variable used to adjust volume when amplitude exceed the limit
    double rate;

    //buffers to hold audio samples from both panels to actual processing
    double buffer1[960], buffer2[960];

signals:

    //signals to run proper actions if enabled
    void lowEQChange(int value);
    void medEQChange(int value);
    void highEQChange(int value);
    void lowEQChange2(int value);
    void medEQChange2(int value);
    void highEQChange2(int value);
    void crossChange(int value);
    void crossChange2(int value);
    void volumeChange(int value);
    void volumeChange2(int value);
    void tempoChange(int value);
    void tempoChange2(int value);

public slots:
    void play();
    void record();

};

#endif // SOUNDPROCESSING_H
