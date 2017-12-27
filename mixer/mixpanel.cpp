#include "mixpanel.h"
#include "mainwindow.h"

MixPanel::MixPanel(QObject *parent) : QObject(parent)
{
    actPos = 0;
    duration = 0;

    isWhiteNoise = false;
    audioReady = false;
    isPlayed = false;
    plot = false;

    channel1 = new QByteArray();
    channel2 = new QByteArray();

    QAudioFormat format;
    format.setChannelCount(2);
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);
    format.setSampleRate(48000);
    format.setSampleSize(16);

    decoder = new QAudioDecoder;
    decoder->setAudioFormat(format);

    connect(decoder, SIGNAL(bufferReady()), this, SLOT(readBuffer()));
    connect(decoder, SIGNAL(finished()), this, SLOT(finishDecoding()));

    memset(&lowMemEq, 0, sizeof(memEQ));
    memset(&medMemEq, 0, sizeof(memEQ));
    memset(&medMemEq2, 0, sizeof(memEQ));
    memset(&highMemEq, 0, sizeof(memEQ));

    lowEQ(50);
    medEQ(50);
    highEQ(50);


}

void MixPanel::playPause() {
    if(audioReady)
    {
        isPlayed = !isPlayed;

    }
}

void MixPanel::process(double *buffer, int nFrames) {

    if(!audioReady || !isPlayed) {
        memset(buffer, 0, sizeof(double)*nFrames*2);
        return;
    }

    for(int i = 0; i < nFrames; i++) {

        if(actPos >= channel1->size()/sizeof(qint16)) {
            buffer[i*2] = 0;
            buffer[i*2+1] = 0;
            continue;
        }

        qint16 value = *(reinterpret_cast<qint16*>(channel1->data())+actPos);
        //qDebug() << value ;
        double y = processLow(value);
        y = processMedium(y);
        buffer[i*2] = processHigh(y);

        value = *(reinterpret_cast<qint16*>(channel2->data())+actPos);

        y = processLow(value);
        y = processMedium(y);
        buffer[i*2+1] = processHigh(y);


        actPos++;
    }

    if(actPos >= channel1->size()/sizeof(qint16)) {
        actPos = 0;
        isPlayed = false;
    }

    if(isWhiteNoise) {
        for(unsigned int i = 0; i < nFrames*2; i++) {
            buffer[i] = rand();
        }
    }

    int seconds = actPos/48000.;
    int minutes = seconds/60.;
    seconds -= minutes*60;

    QString sSeconds = QString::number(seconds);
    if(sSeconds.size() == 1)
        sSeconds.insert(0, '0');

    QString time = QString::number(minutes) + ":" + sSeconds;

    minutes = duration/1000000./60.;
    seconds = (duration - minutes*1000000*60)/1000000.;

    sSeconds = QString::number(seconds);
    if(sSeconds.size() == 1)
        sSeconds.insert(0, '0');

    time  += "/" + QString::number(minutes) + ":" + QString::number(seconds);

    emit timeChange(time);
}

double MixPanel::processEQ(double sample, memEQ &eq) {

    double y = eq.b0*sample + eq.b1*eq.xmem1 + eq.b2*eq.xmem2 - eq.a1*eq.ymem1 - eq.a2*eq.ymem2;

    eq.xmem2 = eq.xmem1;
    eq.xmem1 = sample;
    eq.ymem2 = eq.ymem1;
    eq.ymem1 = y;

    return y;
}

double MixPanel::processLow(double sample) {
    return processEQ(sample, lowMemEq);
}

double MixPanel::processMedium(double sample) {
    return processEQ(processEQ(sample, medMemEq), medMemEq2);
}

double MixPanel::processHigh(double sample) {
    return processEQ(sample, highMemEq);
}

void MixPanel::shelfFilter(double F0, double g, QString type, memEQ &eq) {

    if(type != "low" && type != "high") {
        qDebug() << "Wrong shelf filter type!";
        return;
    }

    double Fs = 48000;
    double S = 1;

    double A = pow(10, g/40.);
    double w0 = 2*3.14159265359*F0/Fs;
    double alpha = sin(w0)/2. * sqrt( (A + 1/A)*(1/S - 1) + 2 );

    int coef = type=="low"?1:-1;

    eq.b0 =        A*( (A+1) - coef*(A-1)*cos(w0) + 2*sqrt(A)*alpha );
    eq.b1 = 2*coef*A*( (A-1) - coef*(A+1)*cos(w0)                   );
    eq.b2 =        A*( (A+1) - coef*(A-1)*cos(w0) - 2*sqrt(A)*alpha );
    eq.a0 =            (A+1) + coef*(A-1)*cos(w0) + 2*sqrt(A)*alpha;
    eq.a1 =  -2*coef*( (A-1) + coef*(A+1)*cos(w0)                   );
    eq.a2 =            (A+1) + coef*(A-1)*cos(w0) - 2*sqrt(A)*alpha;

    eq.b0 /= eq.a0;
    eq.b1 /= eq.a0;
    eq.b2 /= eq.a0;
    eq.a1 /= eq.a0;
    eq.a2 /= eq.a0;
}

void MixPanel::lowEQ(int value) {
    shelfFilter(500, (value-50)/50.*10, "low", lowMemEq);
}

void MixPanel::medEQ(int value) {
    double g = (value-50)/50.*10;
    shelfFilter(15000, g, "low", medMemEq);
    shelfFilter(500, -g, "low", medMemEq2);
}

void MixPanel::highEQ(int value) {
    shelfFilter(15000, (value-50)/50.*10, "high", highMemEq);
}

void MixPanel::loadAudio(QString filename) {
    isPlayed = false;
    audioReady = false;

    duration = 0;
    actPos = 0;

    channel1->clear();
    channel2->clear();

    decoder->stop();
    decoder->setSourceFilename(filename);
    decoder->start();
}

void MixPanel::readBuffer() {
    QAudioBuffer buffer = decoder->read();
    duration += buffer.duration();

    const qint16 *data = buffer.constData<qint16>();

    for(int i = 0; i < buffer.sampleCount()/2; i++) {
        channel1->append(reinterpret_cast<const char*>(data+i*2), sizeof(qint16));
        channel2->append(reinterpret_cast<const char*>(data+1+i*2), sizeof(qint16));
    }
}

void MixPanel::finishDecoding() {
    qDebug() << "ready";
    plot = false;
    audioReady = true;

    int minutes = duration/1000000./60.;
    int seconds = (duration - minutes*1000000*60)/1000000.;

    QString sSeconds = QString::number(seconds);
    if(sSeconds.size() == 1)
        sSeconds.insert(0, '0');

    QString time  = "0:00/" + QString::number(minutes) + ":" + QString::number(seconds);

    emit timeChange(time);
}

void MixPanel::enableWhiteNoise() {
    isWhiteNoise = true;
}

void MixPanel::disableWhiteNoise() {
    isWhiteNoise = false;
}

MixPanel::~MixPanel() {
    delete decoder;

    delete channel1;
    delete channel2;
}






