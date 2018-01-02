#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QThread>
#include <QTimer>
#include "qcustomplot.h"
#include "soundprocessing.h"
#include "mixpanel.h"

namespace Ui {

class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0); 
    QTimer dataTimer;
    QTimer dataTimer2;

    void setupSoundGraph(QCustomPlot *customPlot);
    void setupSoundGraph2(QCustomPlot *customPlot);
    QCPBars *bars1;
    QVector<double> x1,y2;
    QCPBars *bars2;
    QVector<double> x2,y1;

    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    SoundProcessing* soundProc;
    QThread th_soundProc;

signals:
    void loadAudio(QString filename);
    void loadAudio2(QString filename);
    void saveActionToFile(QString fileName);
    void loadActionFromFile(QString fileName);

public slots:
    //----PLOTING------------
     void bracketDataSlot();
     void graphClicked(QCPAbstractPlottable *plottable, int dataIndex);
     void bracketDataSlot2();
     void graphClicked2(QCPAbstractPlottable *plottable, int dataIndex);
    //--------------------
    void selectAudio();
    void selectAudio2();
    void crossFaderChange(int value);
    void saveAction();
    void loadAction();

    void lowChange(int value);
    void medChange(int value);
    void highChange(int value);
    void crossChanger(int value);

};

#endif // MAINWINDOW_H
