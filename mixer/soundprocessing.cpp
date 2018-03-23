#include "soundprocessing.h"
//============================================================
SoundProcessing::SoundProcessing(QObject *parent) : QObject(parent)
{
    isRecording = false; //enable/disable recording of output

    rate = 1; //variable used to adjust volume when amplitude exceed the limit
    crossFader = 50; //value range <0,100>, 50 means equal volume from both channels

    //every music file is decoded to this format
    format.setChannelCount(2);
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);
    format.setSampleRate(48000);
    format.setSampleSize(16);

    //set audio output device
    audioOutput = new QAudioOutput(QAudioDeviceInfo::defaultOutputDevice(), format);
    audioOutput->setBufferSize(9600);
    audioDevice = audioOutput->start();

    //audio processing is executed every 10ms to not overload CPU
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(play()));
    timer->setInterval(10);
    timer->start();
}
//------------------------------------------------------------
void SoundProcessing::play() {

    //array to mix sound from both panels and output
    QByteArray output;

    //get apropriate number of music samples from both panels
    panel1.process(buffer1, 480);
    panel2.process(buffer2, 480);

    //launch apropriate actions for actual time if enabled
    launchActions(panel1.actPos, panel2.actPos);

    //mix sound from both panels, and adjust volume according to crossfader
    for(int i = 0; i < 960; i++) {
        //increase volume limit fluently
        rate += 0.000001;
        if(rate > 1) rate = 1;

        //<= 50 - start decrease panel 2 volume
        //> 50 - start decrease panel 1 volume
        if(crossFader <= 50)
            buffer2[i] *= crossFader/50.;
        else
            buffer1[i] *= 1-(crossFader - 50)/50.;

        double y = buffer1[i] + buffer2[i];

        //if volume exceeds limit decrease it
        if(abs(y)*rate > 30000) {
            rate = abs(30000/y);
        }

        qint16 sample = y*rate;
        output.append(reinterpret_cast<char*>(&sample), sizeof(qint16));
    }

    //if recording is enabled, save samples
    if(isRecording)
        output1.insert(output1.size(),output);

    //output samples
    qint64 written = 0;
    while((written += audioDevice->write(output.right(output.size()-written))) < output.size());
}
//---------------------------------------------
//enable/disable recording
void SoundProcessing::record() {
    if(!isRecording)
        output1.clear();

    isRecording = !isRecording;
}
//---------------------------------------------
void SoundProcessing::launchActions(quint64 actPos1, quint64 actPos2)
{
    if(action.actionLoaded && action.isRunning){  //check if buffer is ready
        if(action.loadBuffer[0] > 0) { //is there any actions for panel 1?
            if(action.p1 >= action.beg2) //check if pointer is not behind his position
                action.p1 = action.beg2-3; //if it is set it to last element

            //set pointers in proper place
            while(action.p1 > action.beg1 && actPos1 < action.loadBuffer[action.p1+1]) {
                action.p1-=3;
            }

            while(action.p1 < action.beg2-3 && actPos1 >= action.loadBuffer[action.p1+4]) {
                action.p1+=3;
            }

            //run actions from proper range
            while(action.p1 < action.beg2 && actPos1 >= action.loadBuffer[action.p1+1] && actPos1-512 < action.loadBuffer[action.p1+1])
            {
                int type = action.loadBuffer[action.p1];

                //launch proper action
                switch (type) {
                case low:
                    emit lowEQChange(action.loadBuffer[action.p1+2]);
                    action.p1 += 3;
                    break;
                case med:
                    emit medEQChange(action.loadBuffer[action.p1+2]);
                    action.p1 += 3;
                    break;
                case high:
                    emit highEQChange(action.loadBuffer[action.p1+2]);
                    action.p1 += 3;
                    break;
                case cross:
                    crossFader = action.loadBuffer[action.p1+2];
                    emit crossChange(action.loadBuffer[action.p1+2]);
                    action.p1 += 3;
                    break;
                case volume:
                    emit volumeChange(action.loadBuffer[action.p1+2]);
                    action.p1 += 3;
                    break;
                case tempo:
                    emit tempoChange(action.loadBuffer[action.p1+2]);
                    action.p1 += 3;
                    break;
                default:
                    break;
                }
            }
        }

        if(action.beg2 < action.loadBuffer.size()) { //is there any actions for panel 2?
            if(action.p2 >= action.loadBuffer.size()) //check if pointer is not behind his position
                action.p2 = action.loadBuffer.size()-3; //if it is set it to last element

            //set pointers in proper place
            while(action.p2 > action.beg2 && actPos2 < action.loadBuffer[action.p2+1]) {
                action.p2-=3;
            }

            while(action.p2 < action.loadBuffer.size()-3 && actPos2 >= action.loadBuffer[action.p2+4]) {
                action.p2+=3;
            }

            //run actions from proper range
            while(action.p2 < action.loadBuffer.size() && actPos2 >= action.loadBuffer[action.p2+1] && actPos2-512 < action.loadBuffer[action.p2+1])
            {
                int type = action.loadBuffer[action.p2];

                //run actions from proper range
                switch (type) {
                case low:
                    emit lowEQChange2(action.loadBuffer[action.p2+2]);
                    action.p2 += 3;
                    break;
                case med:
                    emit medEQChange2(action.loadBuffer[action.p2+2]);
                    action.p2 += 3;
                    break;
                case high:
                    emit highEQChange2(action.loadBuffer[action.p2+2]);
                    action.p2 += 3;
                    break;
                case cross:
                    crossFader = action.loadBuffer[action.p2+2];
                    emit crossChange(action.loadBuffer[action.p2+2]);
                    action.p2 += 3;
                    break;
                case volume:
                    emit volumeChange2(action.loadBuffer[action.p2+2]);
                    action.p2 += 3;
                    break;
                case tempo:
                    emit tempoChange2(action.loadBuffer[action.p2+2]);
                    action.p2 += 3;
                    break;
                default:
                    break;
                }
            }
        }
    }
}
