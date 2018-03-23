#ifndef MIXPANEL_H
#define MIXPANEL_H

#include <QObject>
#include <QAudioFormat>
#include <QAudioBuffer>
#include <QAudioDecoder>
#include "math.h"
#include <QMainWindow>
#include <QFileDialog>
#include <QThread>
#include <QTimer>
#include "qcustomplot.h"
#include "action.h"
#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include "soundtouch/BPMDetect.h"

extern QReadWriteLock lock;

class MixPanel : public QObject
{
    Q_OBJECT

private:
    //struct for EQ timeline filters
    //http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
    struct memEQ {
        double xmem1, xmem2, ymem1, ymem2;
        double b0, b1, b2, a0, a1, a2;
    };

    //used to decode audio to raw
    QAudioDecoder *decoder;

    //play/pause
    bool isPlayed;

    //repeat audio
    bool isSingleLoop;

    //is BPM detected
    bool isBPM;

    //is someone uses disc
    bool isDisc;

    //detected bpm
    double bpm;

    //const to properly position pointer on sound graph
    int  rewindConst;
    int  audioLengthInSec;

    //calculated interval for looping
    int loopInterval;

    //start position of looping
    int loopStart;

    //actual looping type 1/2, 1/4 etc.
    int actLoop;

    //samples to play faster/slower using disc
    int discSamples;

    //used to calculate moved angle
    float prevDiscAngle;
    float discSpeed;

    //audio speed
    double speed;

    //audio volume
    qreal volume;

    //EQ stuctures for filter low/med/high tones
    //for every tones we need 2 filter, for example one for stronger
    //low tones and one for weaker low tones
    memEQ lowMemEq[2];
    //for med tones we need 2 filters for stronger and two for weaker tones
    memEQ medMemEq[2];
    memEQ medMemEq2[2];
    memEQ highMemEq[2];

    //slider values of EQ
    double lowValue;
    double medValue;
    double highValue;

public:
    explicit MixPanel(QObject *parent = nullptr);
    ~MixPanel();

    bool plot;

    //is audio decoded
    bool audioReady;

    //large loop states
    bool isLoopStartSet;
    bool isLoopEndSet;
    bool isLoopingActive;
    bool isLoopingSet;

    //is flag to remember position set
    bool flags[4];

    //if new audio starts loading and old hasn't loaded to the end
    bool loadAudioInterruption;

    //used to set pointer on sound graph while dragging
    double rewindParam;

    //length in miliseconds from taglib
    double audioLength;

    //big loop start/stop position
    qint64 loopingStart;
    qint64 loopingEnd;

    //flags positions
    qint64 flagPos[4];

    //duration in number of samples
    qint64 duration;

    //number of actual sample
    qint64 actPos;

    //used because of speed change, is casted to actPos
    qreal realPosition;

    //audio separated to left and right channel
    QByteArray *channel1;
    QByteArray *channel2;

    //http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
    //computate filter for actual sample
    double processEQ(double sample, memEQ &eq);

    //functions to compute filtered sound with stronger/weaker lows, meds and highs
    //they are combined to output equalized sound
    double processLowUp(double sample);
    double processLowDown(double sample);
    double processMediumUp(double sample);
    double processMediumDown(double sample);
    double processHighUp(double sample);
    double processHighDown(double sample);

    //returns nFrames frames used to output, used to mix raw audio with effects
    void process(double *buffer, int nFrames);

    //create filters for EQ
    void shelfFilter(double F0, double g, QString type, memEQ &eq);
    void detectBPM();

    //set looping
    //arguments: 1 -> 1/16, 2 -> 1/8, 3 -> 1/4 ... 6 -> 2
    void setLoop(int loop);

    //set/unset flag, arguments <1,4>
    void setFlag(int flag);
    void unsetFlag(int flag);

signals:
    //change actual time label
    void timeChange(QString time);

    //signal to save action
    void writeToFile(quint64 type, quint64 position, quint64 value);

    //decoding has finished
    void fileReady();
    void pause();

public slots:
    void playPause();
    void playStop();

    //big loop settings
    void playLoop();
    void playLoopingSet();
    void playLoopingStart();
    void playLoopingEnd();

    //calculate actual disc rotation speed
    void getDiscSpeed(float angle);

    //disc is used/not used
    void enableDisc();
    void disableDisc();

    //set/unset proper flag
    void setFlag1();
    void setFlag2();
    void setFlag3();
    void setFlag4();

    void unsetFlag1();
    void unsetFlag2();
    void unsetFlag3();
    void unsetFlag4();

    //set looping
    void setLoop1_16();
    void setLoop1_8();
    void setLoop1_4();
    void setLoop1_2();
    void setLoop1();
    void setLoop2();

    //start to decode audio
    void loadAudio(QString filename);

    //small part of decoded audio to read
    void readBuffer();
    void finishDecoding();

    //change EQ
    void lowEQ(int value);
    void medEQ(int value);
    void highEQ(int value);

    void speedChange(int value);
    void volumeChange(int value);
};

#endif // MIXPANEL_H
