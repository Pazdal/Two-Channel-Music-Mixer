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

    QByteArray output;

    panel1.process(buffer1, 128);
    panel2.process(buffer2, 128);


    //--------------------ACTION!!!!!!!!!!---------------------------------
    //Tutaj wszystko sie odpierdziela
    if(!action.loadBuffer.empty()){  //sprawdzam czy jest pusty bufor z wczytanymi akcjami
        static int i=0;
        if(panel1.actPos < action.loadBuffer[1])    //jezeli przesune utwor na poczatek to ustawiam ponowne wykonywanie sie akcji
            i = 0;

        //Sposob zapisu akcji:
        //loadBuffer[i] - typ akcji
        //loadBuffer[i+1] - pozycja utworu w ktorej ma sie wykonac akcja
        //loadBuffer[i+2] - wartosc zmiany

        int type = action.loadBuffer[i];
        //switch od tego co ma sie zmieniac
        switch (type) {
        case white_noise_true:
            //biore zakres +-128 od aktualnej pozycji utworu zeby nie bylo sytuacji ze nie trafil tutaj w zapisana w akcji
            //pozycje i nie utknal w switchu :)
            if((panel1.actPos - 128) <= action.loadBuffer[i+1] && (panel1.actPos+ 128) >= action.loadBuffer[i+1]){
                panel1.isWhiteNoise = true;
                i += 3; //przesuwam o 3 zeby dostac sie do kolejnego typu akcji
            }
            break;
        case white_noise_false:
            if((panel1.actPos - 128) <= action.loadBuffer[i+1] && (panel1.actPos+ 128) >= action.loadBuffer[i+1]){
                panel1.isWhiteNoise = false;
                i += 3;
            }
            break;
        case low:
            if((panel1.actPos - 128) <= action.loadBuffer[i+1] && (panel1.actPos+ 128) >= action.loadBuffer[i+1]  ){
                panel1.lowEQ(action.loadBuffer[i+2]);
                //emituje sygnal do zmiany suwaka
                emit lowEQChange(action.loadBuffer[i+2]);
                 i += 3;
            }
            break;
        case med:
            if((panel1.actPos - 128) <= action.loadBuffer[i+1] && (panel1.actPos+ 128) >= action.loadBuffer[i+1]){
                panel1.medEQ(action.loadBuffer[i+2]);
                emit medEQChange(action.loadBuffer[i+2]);
                i += 3;
            }
            break;
        case high:
            if((panel1.actPos - 128) <= action.loadBuffer[i+1] && (panel1.actPos+ 128) >= action.loadBuffer[i+1]){
                panel1.highEQ(action.loadBuffer[i+2]);
                emit highEQChange(action.loadBuffer[i+2]);
                i += 3;
            }
            break;
        case cross:
            if((panel1.actPos - 128) <= action.loadBuffer[i+1] && (panel1.actPos+ 128) >= action.loadBuffer[i+1]){
                crossFader = action.loadBuffer[i+2];
                emit crossChange(action.loadBuffer[i+2]);
                i += 3;
            }
            break;
        default:
            break;
        }
    }
    else qDebug() << "EMPTY ACTION";



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
