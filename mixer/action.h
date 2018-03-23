#ifndef ACTION_H
#define ACTION_H

#include <QObject>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <vector>
#include <QDebug>

//class used to control actions
class Action : public QObject
{
    Q_OBJECT

private:
    //buffers for actions for both panels
    QVector<quint64> buffer;
    QVector<quint64> buffer2;

    //record action enabled/disabled
    bool isRecording;

public:
    Action();
    //buffer for loaded actions
    QVector<quint64> loadBuffer;
    //pointers to actual action/first action for both panels
    int p1, p2, beg1, beg2;

    //is action loaded from file? is enabled?
    bool actionLoaded;
    bool isRunning;

signals:

public slots:
    void saveActionToFile(QString fileName);

    //save panel 1,2 actions to buffer
    void writePanel1(quint64 type, quint64 position, quint64 value);
    void writePanel2(quint64 type, quint64 position, quint64 value);

    void loadActionFromFile(QString fileName);
    void record();
    void run();
};

#endif // ACTION_H
