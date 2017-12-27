#include "soundprocessing.h"
//============================================================
SoundProcessing::SoundProcessing(QObject *parent) : QObject(parent)
{
    abort = false;
    rate = 1;
    crossFader = 50;

    QAudioFormat format;
    format.setChannelCount(2);
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);
    format.setSampleRate(48000);
    format.setSampleSize(16);

    audioOutput = new QAudioOutput(QAudioDeviceInfo::defaultOutputDevice(), format);
    audioDevice = audioOutput->start();

    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(play()));
    timer->start(1);
}
//------------------------------------------------------------
void SoundProcessing::play() {

    if(abort) timer->stop();
    if(!timer->isActive()) return;

    //double buffer1[128], buffer2[128];

    QByteArray output;

    panel1.process(buffer1, 128);
    panel2.process(buffer2, 128);



    for(int i = 0; i < 256; i++) {
        rate += 0.0000001;
        if(rate > 1) rate = 1;

        if(crossFader <= 50)
            buffer2[i] *= crossFader/50.;
        else
            buffer1[i] *= 1-(crossFader - 50)/50.;

        double y = buffer1[i] + buffer2[i];

        if(abs(y)*rate > 30000) {
            rate = abs(30000/y);
        }

        qint16 sample = y*rate;
        output.append(reinterpret_cast<char*>(&sample), sizeof(qint16));
    }

    qint64 written = 0;
    while((written += audioDevice->write(output.right(output.size()-written))) < output.size());
}
