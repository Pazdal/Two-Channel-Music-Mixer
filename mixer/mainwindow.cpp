#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <QMessageBox>
#include <QMetaEnum>
//===============================================
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupSoundGraph(ui->customPlot);
    setupSoundGraph2(ui->customPlot_2);

    soundProc = new SoundProcessing;
    soundProc->moveToThread(&th_soundProc);

    connect(&th_soundProc, &QThread::finished, soundProc, &QObject::deleteLater);
    th_soundProc.start();

    connect(ui->actionQuit, SIGNAL(triggered(bool)), this, SLOT(close()));

    connect(ui->pbAddMusic, SIGNAL(clicked(bool)), this, SLOT(selectAudio()));
    connect(ui->action_Open, SIGNAL(triggered(bool)), this, SLOT(selectAudio()));
    connect(this, SIGNAL(loadAudio(QString)), &soundProc->panel1, SLOT(loadAudio(QString)));
    connect(ui->pbYes, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(enableWhiteNoise()));
    connect(ui->pbNo, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(disableWhiteNoise()));
    connect(ui->pbSound, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(playPause()));

    connect(ui->sLow, SIGNAL(sliderMoved(int)), &soundProc->panel1, SLOT(lowEQ(int)));
    connect(ui->sMedium, SIGNAL(sliderMoved(int)), &soundProc->panel1, SLOT(medEQ(int)));
    connect(ui->sHigh, SIGNAL(sliderMoved(int)), &soundProc->panel1, SLOT(highEQ(int)));

    //AKCJE
    //sygnaly do zmiany suwakow
    connect(soundProc, SIGNAL(lowEQChange(int)), this, SLOT(lowChange(int)));
    connect(soundProc, SIGNAL(medEQChange(int)), this, SLOT(medChange(int)));
    connect(soundProc, SIGNAL(highEQChange(int)), this, SLOT(highChange(int)));
    connect(soundProc, SIGNAL(crossChange(int)), this, SLOT(crossChanger(int)));

    connect(&soundProc->panel1, SIGNAL(timeChange(QString)), ui->lTime, SLOT(setText(QString)));

    connect(ui->pbAddMusic_2, SIGNAL(clicked(bool)), this, SLOT(selectAudio2()));
    connect(ui->action_Open2, SIGNAL(triggered(bool)), this, SLOT(selectAudio2()));
    connect(this, SIGNAL(loadAudio2(QString)), &soundProc->panel2, SLOT(loadAudio(QString)));
    connect(ui->pbYes_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(enableWhiteNoise()));
    connect(ui->pbNo_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(disableWhiteNoise()));
    connect(ui->pbSound_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(playPause()));

    connect(ui->sLow_2, SIGNAL(sliderMoved(int)), &soundProc->panel2, SLOT(lowEQ(int)));
    connect(ui->sMedium_2, SIGNAL(sliderMoved(int)), &soundProc->panel2, SLOT(medEQ(int)));
    connect(ui->sHigh_2, SIGNAL(sliderMoved(int)), &soundProc->panel2, SLOT(highEQ(int)));

    connect(&soundProc->panel2, SIGNAL(timeChange(QString)), ui->lTime_2, SLOT(setText(QString)));

    connect(ui->sCrossfader, SIGNAL(sliderMoved(int)), this, SLOT(crossFaderChange(int)));


    connect(ui->customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*,int)));
    connect(ui->customPlot_2, SIGNAL(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(graphClicked2(QCPAbstractPlottable*,int)));

    //AKCJE
    //sygnaly do zapisywania i wczytywania akcji
    connect(ui->pbActionSave, SIGNAL(clicked(bool)), this, SLOT(saveAction()));
    connect(this, SIGNAL(saveActionToFile(QString)), &soundProc->action, SLOT(saveActionToFile(QString)));

    connect(ui->pbActionLoad, SIGNAL(clicked(bool)), this, SLOT(loadAction()));
    connect(this, SIGNAL(loadActionFromFile(QString)), &soundProc->action, SLOT(loadActionFromFile(QString)));

    connect(&soundProc->panel1, SIGNAL(writeToFile(quint64,quint64,quint64)), &soundProc->action, SLOT(write(quint64, quint64,quint64)));
}
//------------------------------------------------------------
void MainWindow::setupSoundGraph(QCustomPlot *customPlot)
{

  customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
  QCPGraph *graph = customPlot->addGraph();

  //stworzenie i ustawienie wskaznika do pokazywania aktualnego czasu utworu
  bars1 = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis);
  bars1->setWidth(0.025);
  bars1->setPen(QPen(QColor(Qt::green)));
  bars1->setBrush(QBrush(QBrush(QColor(Qt::green))));

  //stworzenie tickera do ustawienia osiX na minuty z sekundami
  QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
  timeTicker->setTimeFormat("%m:%s");
  customPlot->xAxis->setTicker(timeTicker);
  customPlot->axisRect()->setupFullAxesBox();
  customPlot->yAxis->setRange(-75000, 75000);   //ustawienie zakresu osiY
    customPlot->yAxis->setVisible(false);
    customPlot->yAxis->setOffset(0); //nie da sie mniejszego :(
    customPlot->yAxis->setPadding(0);
    customPlot->yAxis->setLabelPadding(0);
    customPlot->yAxis->setTickLabelPadding(0);
    customPlot->yAxis2->setVisible(false);
    customPlot->xAxis2->setVisible(false);
    customPlot->xAxis->setVisible(false);

    //tworze po dwa elementy dla vectora odpowiadajacego za wskaznik pozycji utworu,
    //jeden el dla czesci dodatniej wskzanika, drugi dla ujemnej
    x1.push_back(0);
    x1.push_back(0);
    y2.push_back(75000);
    y2.push_back(-75000);

  connect(&dataTimer, SIGNAL(timeout()), this, SLOT(bracketDataSlot()));

  dataTimer.start(0); // ustawinie timera do odswiezania danych
}

void MainWindow::bracketDataSlot()
{
    //jezeli utwor zaladoany to rysuje wykres
  if(soundProc->panel1.audioReady && soundProc->panel1.plot == false){

      int n = soundProc->panel1.channel1->size();
      QVector<double> x(n), y(n);

    //QCPBars *bars1 = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis);
    //bars1->setWidth(100);
    //bars1->setWidth(3);  //to moze byc do zaznaczania :)
    //bars1->setWidth(0.025);
   // bars1->setPen(QPen(QColor(Qt::green)));
   // bars1->setBrush(QBrush(QBrush(QColor(Qt::green))));

      quint64 actPos = 0;

  for (int i=1; i<n/sizeof(qint16); i += 100)
    {
      x[i] = i/48000.;  //dziele przez czestotliwosc zeby zsynchronizowac osX z czasem
      //pobieram i dodaje probki z kanalu 1 i 2, nie wiem czy to jest dobre ale nie bedziemy
      //chyba rysowac dwoch kanalow osobno xD
      y[i] = *(reinterpret_cast<qint16*>(soundProc->panel1.channel1->data())+actPos)
              +*(reinterpret_cast<qint16*>(soundProc->panel1.channel2->data())+actPos);
      actPos += 100;

  }

  //ustawiam dane do rysowania i wskaznik pozycji utworu na 0
  ui->customPlot->graph()->setData(x, y);
  x1[0] = 0;
  y2[0] = 75000;
  x1[1] = 0;
  y2[1] = -75000;
  bars1->setData(x1, y2);
  ui->customPlot->xAxis->setVisible(true);
  ui->customPlot->replot();
  soundProc->panel1.plot = true;
  }


  if(soundProc->panel1.isPlayed){

    //to odpowiada za przesuwanie sie wykresu podczas odtwarzania
      //pobieram aktualna pozycje utworu i dziele przez czestotliwosc zeby dostac sekundy
    ui->customPlot->xAxis->setRange(soundProc->panel1.actPos/48000+2, 4, Qt::AlignRight);

    x1[0] = soundProc->panel1.actPos/48000;
    x1[1]=  soundProc->panel1.actPos/48000;
    y2[0] = 75000;
    y2[1] = -75000;
    bars1->setData(x1, y2);

    ui->customPlot->replot();
  }

}


void MainWindow::graphClicked(QCPAbstractPlottable *plottable, int dataIndex)
{
  double czas = plottable->interface1D()->dataMainKey(dataIndex);  //TO JEST OS X - sekundy
  double amplituda = plottable->interface1D()->dataMainValue(dataIndex);  //TO JEST OS Y - amplituda dzwieku

  soundProc->panel1.actPos = czas*48000;
  x1[0] = czas;
  x1[1] = czas;
  y2[0] = 75000;
  y2[1] = -75000;
  bars1->setData(x1, y2);
  ui->customPlot->replot();

}
//-----------GRAPH 1--------------------------------------------------

void MainWindow::setupSoundGraph2(QCustomPlot *customPlot)
{

  customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
  QCPGraph *graph = customPlot->addGraph();

  //stworzenie i ustawienie wskaznika do pokazywania aktualnego czasu utworu
  bars2 = new QCPBars(ui->customPlot_2->xAxis, ui->customPlot_2->yAxis);
  bars2->setWidth(0.025);
  bars2->setPen(QPen(QColor(Qt::green)));
  bars2->setBrush(QBrush(QBrush(QColor(Qt::green))));

  //stworzenie tickera do ustawienia osiX na minuty z sekundami
  QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
  timeTicker->setTimeFormat("%m:%s");
  customPlot->xAxis->setTicker(timeTicker);
  customPlot->axisRect()->setupFullAxesBox();
  customPlot->yAxis->setRange(-75000, 75000);   //ustawienie zakresu osiY
    customPlot->yAxis->setVisible(false);
    customPlot->yAxis->setOffset(0); //nie da sie mniejszego :(
    customPlot->yAxis->setPadding(0);
    customPlot->yAxis->setLabelPadding(0);
    customPlot->yAxis->setTickLabelPadding(0);
    customPlot->yAxis2->setVisible(false);
    customPlot->xAxis2->setVisible(false);
    customPlot->xAxis->setVisible(false);

    //tworze po dwa elementy dla vectora odpowiadajacego za wskaznik pozycji utworu,
    //jeden el dla czesci dodatniej wskzanika, drugi dla ujemnej
    x2.push_back(0);
    x2.push_back(0);
    y1.push_back(75000);
    y1.push_back(-75000);

  connect(&dataTimer2, SIGNAL(timeout()), this, SLOT(bracketDataSlot2()));

  dataTimer2.start(0); // ustawinie timera do odswiezania danych
}

void MainWindow::bracketDataSlot2()
{
    //jezeli utwor zaladoany to rysuje wykres
  if(soundProc->panel2.audioReady && soundProc->panel2.plot == false){

      int n = soundProc->panel2.channel1->size();
      QVector<double> x(n), y(n);
      quint64 actPos = 0;

  for (int i=1; i<n/sizeof(qint16); i+=100)
    {
      x[i] = i/48000.;  //dziele przez czestotliwosc zeby zsynchronizowac osX z czasem
      //pobieram i dodaje probki z kanalu 1 i 2, nie wiem czy to jest dobre ale nie bedziemy
      //chyba rysowac dwoch kanalow osobno xD
      y[i] = *(reinterpret_cast<qint16*>(soundProc->panel2.channel1->data())+actPos)
              +*(reinterpret_cast<qint16*>(soundProc->panel2.channel2->data())+actPos);
      actPos += 100;

  }

  //ustawiam dane do rysowania i wskaznik pozycji utworu na 0
  ui->customPlot_2->graph()->setData(x, y);
  x2[0] = 0;
  y1[0] = 75000;
  x2[1] = 0;
  y1[1] = -75000;
  bars2->setData(x2, y1);
  ui->customPlot_2->xAxis->setVisible(true);
  ui->customPlot_2->replot();
  soundProc->panel2.plot = true;
  }


  if(soundProc->panel2.isPlayed){

    //to odpowiada za przesuwanie sie wykresu podczas odtwarzania
      //pobieram aktualna pozycje utworu i dziele przez czestotliwosc zeby dostac sekundy
    ui->customPlot_2->xAxis->setRange(soundProc->panel2.actPos/48000+2, 4, Qt::AlignRight);

    x2[0] = soundProc->panel2.actPos/48000;
    x2[1]=  soundProc->panel2.actPos/48000;
    y1[0] = 75000;
    y1[1] = -75000;
    bars2->setData(x2, y1);

    ui->customPlot_2->replot();
  }

}


void MainWindow::graphClicked2(QCPAbstractPlottable *plottable, int dataIndex)
{
  double czas = plottable->interface1D()->dataMainKey(dataIndex);  //TO JEST OS X - sekundy
  double amplituda = plottable->interface1D()->dataMainValue(dataIndex);  //TO JEST OS Y - amplituda dzwieku

  soundProc->panel2.actPos = czas*48000;
  x2[0] = czas;
  x2[1] = czas;
  y1[0] = 75000;
  y1[1] = -75000;
  bars2->setData(x2, y1);
  ui->customPlot_2->replot();

}
//----------PLOTING------------------------------------------------------



void MainWindow::selectAudio() {
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open file"), "/home/", tr("Music Files (*.mp3)"));

    if(filename == "") return;

    ui->laudio->setText(filename.split('/').last());
    emit loadAudio(filename);
}

void MainWindow::closeEvent(QCloseEvent * /* unused */) {
    soundProc->abort = true;
}
//------------------------------------------------------------
void MainWindow::selectAudio2() {
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open file"), "/home/", tr("Music Files (*.mp3)"));

    if(filename == "") return;

    ui->laudio_2->setText(filename.split('/').last());
    emit loadAudio2(filename);
}
//------------------------------------------------------------
void MainWindow::crossFaderChange(int value) {
    soundProc->crossFader = value;
    emit soundProc->panel1.writeToFile(6, soundProc->panel1.actPos,value);
}

void MainWindow::saveAction()
{
     QString fileName = QFileDialog::getSaveFileName(this,
         tr("Save action"), "/home", tr("save(*.acn)"));

     if(fileName == "") return;

     emit saveActionToFile(fileName);

}

void MainWindow::loadAction()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open action"), "/home", tr("Action Files (*.acn)"));

    if(fileName == "") return;

    emit loadActionFromFile(fileName);
}

void MainWindow::lowChange(int value)
{
    ui->sLow->setSliderPosition(value);
    ui->sLow->setValue(value);
}

void MainWindow::medChange(int value)
{
    ui->sMedium->setSliderPosition(value);
    ui->sMedium->setValue(value);
}

void MainWindow::highChange(int value)
{
    ui->sHigh->setSliderPosition(value);
    ui->sHigh->setValue(value);
}

void MainWindow::crossChanger(int value)
{
    ui->sCrossfader->setSliderPosition(value);
    ui->sCrossfader->setValue(value);
}
//------------------------------------------------------------
MainWindow::~MainWindow()
{
    th_soundProc.quit();
    th_soundProc.wait();
    delete ui;
}

//------------------------------------------------




