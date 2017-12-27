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

    soundProc = new SoundProcessing;
    soundProc->moveToThread(&th_soundProc);

    connect(&th_soundProc, &QThread::finished, soundProc, &QObject::deleteLater);
    th_soundProc.start();

    connect(ui->pbAddMusic, SIGNAL(clicked(bool)), this, SLOT(selectAudio()));
    connect(this, SIGNAL(loadAudio(QString)), &soundProc->panel1, SLOT(loadAudio(QString)));
    connect(ui->pbYes, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(enableWhiteNoise()));
    connect(ui->pbNo, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(disableWhiteNoise()));
    connect(ui->pbSound, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(playPause()));

    connect(ui->sLow, SIGNAL(sliderMoved(int)), &soundProc->panel1, SLOT(lowEQ(int)));
    connect(ui->sMedium, SIGNAL(sliderMoved(int)), &soundProc->panel1, SLOT(medEQ(int)));
    connect(ui->sHigh, SIGNAL(sliderMoved(int)), &soundProc->panel1, SLOT(highEQ(int)));

    connect(&soundProc->panel1, SIGNAL(timeChange(QString)), ui->lTime, SLOT(setText(QString)));

    connect(ui->pbAddMusic_2, SIGNAL(clicked(bool)), this, SLOT(selectAudio2()));
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
    ui->customPlot->yAxis->setVisible(false);
    ui->customPlot->yAxis->setOffset(0); //nie da sie mniejszego :(
    ui->customPlot->yAxis->setPadding(0);
    ui->customPlot->yAxis->setLabelPadding(0);
    ui->customPlot->yAxis->setTickLabelPadding(0);
    ui->customPlot->yAxis2->setVisible(false);
    ui->customPlot->xAxis2->setVisible(false);
    ui->customPlot->xAxis->setVisible(false);

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

  for (int i=0; i<n/sizeof(qint16); ++i)
    {
      x[i] = i/48000.;  //dziele przez czestotliwosc zeby zsynchronizowac osX z czasem
      //pobieram i dodaje probki z kanalu 1 i 2, nie wiem czy to jest dobre ale nie bedziemy
      //chyba rysowac dwoch kanalow osobno xD
      y[i] = *(reinterpret_cast<qint16*>(soundProc->panel1.channel1->data())+actPos)
              +*(reinterpret_cast<qint16*>(soundProc->panel1.channel2->data())+actPos);
      actPos++;

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
}
//------------------------------------------------------------
MainWindow::~MainWindow()
{
    th_soundProc.quit();
    th_soundProc.wait();
    delete ui;
}

//------------------------------------------------




