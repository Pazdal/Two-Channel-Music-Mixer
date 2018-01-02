#include "action.h"

Action::Action(QObject *parent) : QObject(parent)
{

}

void Action::saveActionToFile(QString fileName)
{
    //zapsywanie akcji
    QString name = fileName + ".acn";
    QFile file(name);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);
    for(int i=0; i < buffer.size() ; i++)
    out << buffer[i] << "\n";
    file.close();
}

void Action::write(quint64 type,quint64 position, quint64 value)
{
    //Sposob zapisu akcji:
    //loadBuffer[i] - typ akcji
    //loadBuffer[i+1] - pozycja utworu w ktorej ma sie wykonac akcja
    //loadBuffer[i+2] - wartosc zmiany
    buffer.push_back(type);
    buffer.push_back(position);
    buffer.push_back(value);
}

void Action::loadActionFromFile(QString fileName)
{
    loadBuffer.clear();
    int i = 0;
    QFile file(fileName);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    while(!file.atEnd()){
        QString line = file.readLine();
        loadBuffer.push_back(line.toInt());
        qDebug() << "read " << loadBuffer[i++];
    }
}

