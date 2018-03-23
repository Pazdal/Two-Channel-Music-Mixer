#include "action.h"

Action::Action() {
    actionLoaded = false;
    isRunning = false;
    isRecording = false;
}
//---------------------------------------------------
void Action::saveActionToFile(QString fileName)
{
    //save actions to file, first save panel 1 actions
    QString name = fileName + ".acn";
    QFile file(name);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    out << buffer.size() << "\n";
    for(int i=0; i < buffer.size() ; i++){
        out << buffer[i] << "\n";
    }
    for(int i=0; i < buffer2.size() ; i++){
        out << buffer2[i] << "\n";
    }
    file.close();
}
//---------------------------------------------------
void Action::writePanel2(quint64 type, quint64 position, quint64 value)
{
    if(!isRecording) return;

    //clear all actions which occured in time late than actual
    int i = buffer2.size()-2;
    while(i > 0 && buffer2[i] > position) {
        buffer2.remove(i-1, 3);
        i-=3;
    }
    buffer2.push_back(type);
    buffer2.push_back(position);
    buffer2.push_back(value);
}
//---------------------------------------------------
void Action::writePanel1(quint64 type,quint64 position, quint64 value)
{
    if(!isRecording) return;

    //file structure:
    //loadBuffer[i] - action type
    //loadBuffer[i+1] - actual time position in which action will perform
    //loadBuffer[i+2] - change value

    //clear all actions which occured in time late than actual
    int i = buffer.size()-2;
    while(i > 0 && buffer[i] > position) {
        buffer.remove(i-1, 3);
        i-=3;
    }
    buffer.push_back(type);
    buffer.push_back(position);
    buffer.push_back(value);
}
//---------------------------------------------------
void Action::loadActionFromFile(QString fileName)
{
    //load actions from file to buffer
    actionLoaded = false;
    loadBuffer.clear();

    QFile file(fileName);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    while(!file.atEnd()){
        QString line = file.readLine();
        loadBuffer.push_back(line.toInt());
    }
    beg1 = p1 = 1;
    beg2 = p2 = loadBuffer[0]+1;
    actionLoaded = true;
}
//---------------------------------------------------
void Action::record() {
    if(isRecording) {
        isRecording = false;
    }
    else {
        buffer.clear();
        buffer2.clear();
        isRecording = true;
    }
}
//---------------------------------------------------
void Action::run() {
    isRunning = !isRunning;
}
