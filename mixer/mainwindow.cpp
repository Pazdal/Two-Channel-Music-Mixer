#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <QMessageBox>
#include <QMetaEnum>
#include <QReadWriteLock>

QReadWriteLock lock;

//===============================================
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //window fixed size
    setFixedSize(size());

    //load blue and orange disc images
    QString filename = QDir::currentPath();
    filename.append("/../mixer/disc.png");
    QImage image(filename);
    discImg[0] = new QGraphicsPixmapItem(QPixmap::fromImage(image));

    filename = QDir::currentPath();
    filename.append("/../mixer/disc2.png");
    QImage image2(filename);
    discImg[1] = new QGraphicsPixmapItem(QPixmap::fromImage(image2));

    //set graphics scenes for discs
    for(int i = 0; i < 2; i++) {
        discImg[i]->setTransformationMode(Qt::SmoothTransformation);
        discImg[i]->setTransformOriginPoint(100, 100);
        disc[i] = new Disc();
        disc[i]->setSceneRect(0, 0, 200, 200);
    }

    //add disc images to graphics scenes
    ui->disc->setScene(disc[0]);
    ui->disc->scene()->addItem(discImg[0]);
    //set scene background color to main window color
    ui->disc->setBackgroundBrush(QBrush(QColor(54, 54, 54), Qt::SolidPattern));
    ui->disc->show();

    ui->disc_2->setScene(disc[1]);
    ui->disc_2->scene()->addItem(discImg[1]);
    ui->disc_2->setBackgroundBrush(QBrush(QColor(54, 54, 54), Qt::SolidPattern));
    ui->disc_2->show();

    //load stylesheet for mixer
    QString path = QDir::currentPath();
    path.append("/../mixer/stylesheet.qss");
    QFile File(path);
    File.open(QFile::ReadOnly | QFile::Text);
    QTextStream in(&File);

    QString stylesheet = in.readAll();

    setStyleSheet(stylesheet);

    //set sound wave graphs
    setupSoundGraph(ui->customPlot);
    setupSoundGraph2(ui->customPlot_2);

    //create process involved in mixing music from two panels and
    //used to output audio
    soundProc = new SoundProcessing;
    soundProc->moveToThread(&th_soundProc);
    soundProc->timer->moveToThread(&th_soundProc);

    //create class used to exporting recorded audio
    Export = new Exporting(soundProc);

    //start sound processing
    connect(&th_soundProc, &QThread::finished, soundProc, &QObject::deleteLater);
    th_soundProc.start();

    //connect mixer buttons with slots
    connect(ui->actionQuit, SIGNAL(triggered(bool)), this, SLOT(close()));
    connect(ui->pbAddMusic, SIGNAL(clicked(bool)), this, SLOT(selectAudio()));
    connect(ui->action_Open, SIGNAL(triggered(bool)), this, SLOT(selectAudio()));
    connect(this, SIGNAL(loadAudio(QString)), &soundProc->panel1, SLOT(loadAudio(QString)));
    connect(ui->pbSound, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(playPause()));
    connect(&soundProc->panel1, SIGNAL(pause()), ui->pbSound, SLOT(click()));
    connect(ui->pbStop_1, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(playStop()));

    //large loop, buttons to set start/stop and enable/disable loop
    connect(ui->pbSingleLoop_1, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(playLoop()));
    connect(ui->pbLoopEnable_1, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(playLoopingSet()));
    connect(ui->pbLoopStart_1, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(playLoopingStart()));
    connect(ui->pbLoopEnd_1, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(playLoopingEnd()));

    //buttons to remember music position
    connect(ui->pbFlag1, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(setFlag1()));
    connect(ui->pbFlag2, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(setFlag2()));
    connect(ui->pbFlag3, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(setFlag3()));
    connect(ui->pbFlag4, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(setFlag4()));
    connect(ui->pbUnflag1, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(unsetFlag1()));
    connect(ui->pbUnflag2, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(unsetFlag2()));
    connect(ui->pbUnflag3, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(unsetFlag3()));
    connect(ui->pbUnflag4, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(unsetFlag4()));

    //looping
    connect(ui->pbLoop1_16, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(setLoop1_16()));
    connect(ui->pbLoop1_8, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(setLoop1_8()));
    connect(ui->pbLoop1_4, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(setLoop1_4()));
    connect(ui->pbLoop1_2, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(setLoop1_2()));
    connect(ui->pbLoop1, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(setLoop1()));
    connect(ui->pbLoop2, SIGNAL(clicked(bool)), &soundProc->panel1, SLOT(setLoop2()));

    //slider events
    connect(ui->sLow, SIGNAL(valueChanged(int)), &soundProc->panel1, SLOT(lowEQ(int)));
    connect(ui->sMedium, SIGNAL(valueChanged(int)), &soundProc->panel1, SLOT(medEQ(int)));
    connect(ui->sHigh, SIGNAL(valueChanged(int)), &soundProc->panel1, SLOT(highEQ(int)));
    connect(ui->sSpeed1,SIGNAL(valueChanged(int)),&soundProc->panel1,SLOT(speedChange(int)));
    connect(ui->sVolume1,SIGNAL(valueChanged(int)),&soundProc->panel1,SLOT(volumeChange(int)));

    //disc rotation events
    connect(disc[0], SIGNAL(rotate(float)), this, SLOT(rotate(float)));
    connect(disc[0], SIGNAL(rotate(float)), &soundProc->panel1, SLOT(getDiscSpeed(float)));
    connect(disc[0], SIGNAL(start()), &soundProc->panel1, SLOT(enableDisc()));
    connect(disc[0], SIGNAL(stop()), &soundProc->panel1, SLOT(disableDisc()));

    //saved action events
    connect(soundProc, SIGNAL(lowEQChange(int)), this, SLOT(lowChange(int)));
    connect(soundProc, SIGNAL(medEQChange(int)), this, SLOT(medChange(int)));
    connect(soundProc, SIGNAL(highEQChange(int)), this, SLOT(highChange(int)));
    connect(soundProc, SIGNAL(volumeChange(int)), ui->sVolume1, SLOT(setValue(int)));
    connect(soundProc, SIGNAL(volumeChange2(int)), ui->sVolume2, SLOT(setValue(int)));
    connect(soundProc, SIGNAL(tempoChange(int)), ui->sSpeed1, SLOT(setValue(int)));
    connect(soundProc, SIGNAL(tempoChange2(int)), ui->sSpeed2, SLOT(setValue(int)));
    connect(soundProc, SIGNAL(lowEQChange2(int)), this, SLOT(lowChange2(int)));
    connect(soundProc, SIGNAL(medEQChange2(int)), this, SLOT(medChange2(int)));
    connect(soundProc, SIGNAL(highEQChange2(int)), this, SLOT(highChange2(int)));
    connect(soundProc, SIGNAL(crossChange(int)), this, SLOT(crossChanger(int)));

    //connect mixer buttons with slots for second panel
    connect(&soundProc->panel1, SIGNAL(timeChange(QString)), ui->lTime, SLOT(setText(QString)));
    connect(ui->pbAddMusic_2, SIGNAL(clicked(bool)), this, SLOT(selectAudio2()));
    connect(ui->action_Open2, SIGNAL(triggered(bool)), this, SLOT(selectAudio2()));
    connect(this, SIGNAL(loadAudio2(QString)), &soundProc->panel2, SLOT(loadAudio(QString)));
    connect(ui->pbSound_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(playPause()));
    connect(&soundProc->panel2, SIGNAL(pause()), ui->pbSound_2, SLOT(click()));
    connect(ui->pbStop_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(playStop()));

    //large loop, buttons to set start/stop and enable/disable loop
    connect(ui->pbSingleLoop_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(playLoop()));
    connect(ui->pbLoopEnable_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(playLoopingSet()));
    connect(ui->pbLoopStart_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(playLoopingStart()));
    connect(ui->pbLoopEnd_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(playLoopingEnd()));

    //buttons to remember music position
    connect(ui->pbFlag1_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(setFlag1()));
    connect(ui->pbFlag2_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(setFlag2()));
    connect(ui->pbFlag3_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(setFlag3()));
    connect(ui->pbFlag4_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(setFlag4()));
    connect(ui->pbUnflag1_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(unsetFlag1()));
    connect(ui->pbUnflag2_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(unsetFlag2()));
    connect(ui->pbUnflag3_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(unsetFlag3()));
    connect(ui->pbUnflag4_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(unsetFlag4()));

    //looping
    connect(ui->pbLoop1_16_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(setLoop1_16()));
    connect(ui->pbLoop1_8_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(setLoop1_8()));
    connect(ui->pbLoop1_4_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(setLoop1_4()));
    connect(ui->pbLoop1_2_2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(setLoop1_2()));
    connect(ui->pbLoop1__2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(setLoop1()));
    connect(ui->pbLoop2__2, SIGNAL(clicked(bool)), &soundProc->panel2, SLOT(setLoop2()));

    //slider events
    connect(ui->sLow_2, SIGNAL(valueChanged(int)), &soundProc->panel2, SLOT(lowEQ(int)));
    connect(ui->sMedium_2, SIGNAL(valueChanged(int)), &soundProc->panel2, SLOT(medEQ(int)));
    connect(ui->sHigh_2, SIGNAL(valueChanged(int)), &soundProc->panel2, SLOT(highEQ(int)));
    connect(ui->sSpeed2,SIGNAL(valueChanged(int)),&soundProc->panel2,SLOT(speedChange(int)));
    connect(ui->sVolume2,SIGNAL(valueChanged(int)),&soundProc->panel2,SLOT(volumeChange(int)));

    //disc rotation events
    connect(disc[1], SIGNAL(rotate(float)), this, SLOT(rotate2(float)));
    connect(disc[1], SIGNAL(rotate(float)), &soundProc->panel2, SLOT(getDiscSpeed(float)));
    connect(disc[1], SIGNAL(start()), &soundProc->panel2, SLOT(enableDisc()));
    connect(disc[1], SIGNAL(stop()), &soundProc->panel2, SLOT(disableDisc()));

    connect(&soundProc->panel2, SIGNAL(timeChange(QString)), ui->lTime_2, SLOT(setText(QString)));

    //crossfader event
    connect(ui->sCrossfader, SIGNAL(valueChanged(int)), this, SLOT(crossFaderChange(int)));

    //rec and export events
    connect(ui->pbRec, SIGNAL(clicked(bool)), soundProc, SLOT(record()));
    connect(ui->action_Export, SIGNAL(triggered(bool)), this, SLOT(onExport()));
    connect(this, SIGNAL(startExport(QString)), Export, SLOT(exportFile(QString)));

    //sound wave graph events
    connect(ui->customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*,int)));
    connect(ui->customPlot_2, SIGNAL(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(graphClicked2(QCPAbstractPlottable*,int)));

    //save action events
    connect(ui->pbActionSave, SIGNAL(clicked(bool)), this, SLOT(saveAction()));
    connect(this, SIGNAL(saveActionToFile(QString)), &soundProc->action, SLOT(saveActionToFile(QString)));

    //load action events
    connect(ui->pbActionLoad, SIGNAL(clicked(bool)), this, SLOT(loadAction()));
    connect(this, SIGNAL(loadActionFromFile(QString)), &soundProc->action, SLOT(loadActionFromFile(QString)));

    //run/record action events
    connect(ui->pbRecord, SIGNAL(clicked(bool)), &soundProc->action, SLOT(record()));
    connect(ui->pbRun, SIGNAL(clicked(bool)), &soundProc->action, SLOT(run()));

    //save single action
    connect(&soundProc->panel1, SIGNAL(writeToFile(quint64,quint64,quint64)), &soundProc->action, SLOT(writePanel1(quint64,quint64,quint64)));
    connect(&soundProc->panel2, SIGNAL(writeToFile(quint64,quint64,quint64)), &soundProc->action, SLOT(writePanel2(quint64,quint64,quint64)));

    //on sound wave graph mouse move
    connect(ui->customPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseMove(QMouseEvent*)));
    connect(ui->customPlot_2, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseMove2(QMouseEvent*)));
}
//----------------------GRAPH 1---------------------------------------------
void MainWindow::setupSoundGraph(QCustomPlot *customPlot)
{
    //adding a graph to the sound and looping bars
    customPlot->addGraph();
    customPlot->addGraph();
    customPlot->addGraph();
    customPlot->graph(0)->setPen(QPen(QColor(255, 120, 5)));
    customPlot->setBackground(QColor(54, 54, 54));

    //create and set the pointer to show the current position of the song
    trackPointer = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis);
    trackPointer->setWidth(0.025);
    trackPointer->setPen(QPen(QColor(Qt::white)));
    trackPointer->setBrush(QBrush(QBrush(QColor(Qt::white))));

    QColor color1 = QColor(Qt::red);
    color1.setAlpha(50);
    QColor color2 = QColor(Qt::white);
    color2.setAlpha(20);
    
    //create and set looping flag pointers
    barLoopStart_1 = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis);
    barLoopStart_1->setWidth(0.025);
    barLoopStart_1->setPen(QPen(QColor(color2)));
    barLoopStart_1->setBrush(QBrush(QBrush(color2)));

    barLoopEnd_1 = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis);
    barLoopEnd_1->setWidth(0.025);
    barLoopEnd_1->setPen(QPen(QColor(color2)));
    barLoopEnd_1->setBrush(QBrush(QBrush(color2)));

    color2 = QColor(Qt::green);
    color2.setAlpha(50);

    QColor colors[4];
    colors[0] = QColor(Qt::red);
    colors[1] = QColor(Qt::green);
    colors[2] = QColor(Qt::blue);
    colors[3] = QColor(Qt::yellow);
    //create and set return looping flags
    for(int i = 0; i < 4; i++) {
        returnBar[i] = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis);
        returnBar[i]->setWidth(0.025);
        colors[i].setAlpha(150);
        returnBar[i]->setPen(QPen(QColor(colors[i])));
        returnBar[i]->setBrush(QBrush(QBrush(colors[i])));
        returnBar[i]->setVisible(false);
    }

    customPlot->graph(1)->setPen(QPen(Qt::NoPen));
    customPlot->graph(1)->setBrush(QBrush(QBrush(color1)));
    customPlot->graph(2)->setPen(QPen(Qt::NoPen));
    customPlot->graph(2)->setBrush(QBrush(QBrush(color1)));
    
    //set the range of sound amplitude
    customPlot->yAxis->setRange(-100000, 100000);
    customPlot->yAxis->setVisible(false);
    customPlot->yAxis2->setVisible(false);
    customPlot->xAxis2->setVisible(false);
    customPlot->xAxis->setVisible(false);

    //create two elements for the vector corresponding to the position pointer of the song
    //one part for the positive part of the pointer, the other for the negative part
    x1.push_back(0);
    x1.push_back(0);
    y2.push_back(100000);
    y2.push_back(-100000);
    yStart_1.push_back(100000);
    yStart_1.push_back(-100000);
    xStart_1.push_back(0);
    xStart_1.push_back(0);
    yEnd_1.push_back(100000);
    yEnd_1.push_back(-100000);
    xEnd_1.push_back(0);
    xEnd_1.push_back(0);
    yfill.push_back(100000);
    yfill.push_back(-100000);
    xfill.push_back(0);
    xfill.push_back(0);
    yReturn.push_back(100000);
    yReturn.push_back(-100000);
    xReturn.push_back(0);
    xReturn.push_back(0);

    ui->customPlot->graph(1)->setVisible(false);
    ui->customPlot->graph(2)->setVisible(false);

  //setting the timer to refresh data every 100ms   
  connect(&dataTimer, SIGNAL(timeout()), this, SLOT(bracketDataSlot()));
  dataTimer.start(100);
}
//-------------------------------------------------------
void MainWindow::bracketDataSlot()
{
    //if the song is loading, draw a graph
    if(soundProc->panel1.audioReady)
    {    
        //setting the length of the song using the tagLib library
        ui->customPlot->xAxis->setRange(0, soundProc->panel1.audioLength);
        int n = soundProc->panel1.audioLength * (48000 * 2) + 1;
        //creating static vectors so that you do not have to create them every time you refresh
        static QVector<double> x, y;

      static int prevI = 0;
      //if the chart has been interrupted, reset the vectors
      if(soundProc->panel1.loadAudioInterruption)
      {
          prevI = 0;
          x.clear();
          y.clear();
          ui->customPlot->graph(0)->setData(x, y);
          ui->customPlot->replot();
          soundProc->panel1.loadAudioInterruption = false;
      }
      quint64 actPos = prevI * 3000;

    //main loop for collecting amplitudes of samples and drawing a sound graph
    for (int i=1; actPos<n/sizeof(qint16) && i < 50; i++, prevI++)
      {
        lock.lockForWrite();
        int size = soundProc->panel1.channel1->size()/sizeof(qint16);
        if(actPos > size) {
            lock.unlock();
            break;
        }

        //division by frequency to synchronize the X axis with time
        x.push_back(actPos/48000.);
        //downloading and adding samples from channels 1 and 2
        y.push_back(*(reinterpret_cast<qint16*>(soundProc->panel1.channel1->data())+actPos)
                           +*(reinterpret_cast<qint16*>(soundProc->panel1.channel2->data())+actPos));
        lock.unlock();
        //taking a further sample away at 3000 to speed up drawing
        actPos += 3000;
        //drawing a graph every modulo 96
        if(actPos % 96 == 0)
        {
          ui->customPlot->graph(0)->setData(x, y);
          ui->customPlot->replot();
       }
    }

      ui->customPlot->replot();

      if(actPos>=n/sizeof(qint16)){
          soundProc->panel1.plot = true;
          soundProc->panel1.audioReady = false;
          prevI = 0;
          x.clear();
          y.clear();
      }
    }
    
    //checking and displaying looping flags
    for(int i = 0; i < 4; i++) {
        if(soundProc->panel1.flags[i]) {
            xReturn[0] = xReturn[1] = soundProc->panel1.flagPos[i]/48000;
            returnBar[i]->setData(xReturn, yReturn);
            returnBar[i]->setVisible(true);
            ui->customPlot->replot();
        }
        else {
            returnBar[i]->setVisible(false);
        }
    }

  if(soundProc->panel1.isLoopStartSet){
      xStart_1[0]=soundProc->panel1.loopingStart/48000;
      xStart_1[1]=soundProc->panel1.loopingStart/48000;
      barLoopStart_1->setData(xStart_1, yStart_1);
      barLoopStart_1->setVisible(true);
      ui->customPlot->replot();
  }

  if(soundProc->panel1.isLoopEndSet){
      xEnd_1[0]=soundProc->panel1.loopingEnd/48000;
      xEnd_1[1]=soundProc->panel1.loopingEnd/48000;
      barLoopEnd_1->setData(xEnd_1, yEnd_1);
  }
    
  //setting the filling of the looping area  
  if(soundProc->panel1.isLoopingSet)
  {
      QColor color1;

      if(soundProc->panel1.isLoopingActive)
      {
          color1 = QColor(Qt::red);
          color1.setAlpha(50);
      }
      else
      {
          color1 = QColor(Qt::white);
          color1.setAlpha(50);

      }
      barLoopStart_1->setPen(QPen(QColor(color1)));
      barLoopEnd_1->setPen(QPen(QColor(color1)));
      ui->customPlot->graph(1)->setBrush(QBrush(QBrush(color1)));
      ui->customPlot->graph(2)->setBrush(QBrush(QBrush(color1)));
      barLoopStart_1->setVisible(false);
      barLoopEnd_1->setVisible(false);
      double start =  xStart_1[0]=soundProc->panel1.loopingStart/48000;
      double end =  xEnd_1[0]=soundProc->panel1.loopingEnd/48000;
        xfill[0] = (start);
        yfill[0] = (100000);
        xfill[1]=(end);
        yfill[1] = (100000);
        ui->customPlot->graph(1)->setData(xfill,yfill);

        xfill[0] = start;
        yfill[0] = -100000;
        xfill[1] = end;
        yfill[1] = -100000;
        ui->customPlot->graph(2)->setData(xfill,yfill);

        ui->customPlot->graph(1)->setVisible(true);
        ui->customPlot->graph(2)->setVisible(true);
        ui->customPlot->replot();
  }
  else
  {
      ui->customPlot->graph(1)->setVisible(false);
      ui->customPlot->graph(2)->setVisible(false);
      barLoopEnd_1->setVisible(false);
      ui->customPlot->replot();
  }
    
  //this is responsible for moving the graph during playback
  //the current position of the song is downloaded and the pointer is set
  if(soundProc->panel1.duration > 0) {
    x1[0] = soundProc->panel1.actPos/48000;
    x1[1]=  soundProc->panel1.actPos/48000;
    trackPointer->setData(x1, y2);
    ui->customPlot->replot();
  }
}
//-------------------------------------------------------
void MainWindow::graphClicked(QCPAbstractPlottable *plottable, int dataIndex)
{
  //getting seconds from the point where the graph was clicked  
  double czas = plottable->interface1D()->dataMainKey(dataIndex);  

  //clicking outside the looping area always sets looping to false
  if(czas*48000 < soundProc->panel1.loopingStart || czas*48000 > soundProc->panel1.loopingEnd)
      soundProc->panel1.isLoopingActive = false;
  soundProc->panel1.actPos = czas*48000;
  soundProc->panel1.realPosition = czas*48000;
  x1[0] = czas;
  x1[1] = czas;
  trackPointer->setData(x1, y2);
  ui->customPlot->replot();
}
//-------------------------------------------------------
//dragging the sound graph
void MainWindow::mouseMove(QMouseEvent *event)
{
    if(event->buttons() == Qt::LeftButton)
    {
        //getting a dragged position
        double position = (event->x()-15) * soundProc->panel1.rewindParam/soundProc->panel1.audioLength;
        if(position*48000 < soundProc->panel1.loopingStart || position*48000 > soundProc->panel1.loopingEnd)
            soundProc->panel1.isLoopingActive = false;
     
     //if the position exceeds the left end of the graph, set the pointer to the beginning
     if(position < 0)
     {
         x1[0] = 0;
         x1[1] = 0;
         soundProc->panel1.actPos = 0;
         soundProc->panel1.realPosition = 0;
     }
     //if the position exceeds the right end of the graph, set the pointer to the end
     else if(position > soundProc->panel1.audioLength)
     {
         x1[0] = soundProc->panel1.audioLength-0.1;
         x1[1] = soundProc->panel1.audioLength-0.1;
         soundProc->panel1.actPos = soundProc->panel1.audioLength-0.1;
         soundProc->panel1.realPosition = soundProc->panel1.audioLength-0.1;
     }
     //set the pointer to the dragged position
     else
     {
         x1[0] = position;
         x1[1] = position;
         soundProc->panel1.actPos = position*48000;
         soundProc->panel1.realPosition = position*48000;
     }
     trackPointer->setData(x1, y2);
     ui->customPlot->replot();
    }
}

//----------------------GRAPH 2---------------------------------------------

void MainWindow::setupSoundGraph2(QCustomPlot *customPlot)
{
  customPlot->addGraph();
  customPlot->addGraph();
  customPlot->addGraph();
  customPlot->graph(0)->setPen(QPen(QColor(66, 134 ,244)));
  customPlot->setBackground(QColor(54, 54, 54));

  trackPointer2 = new QCPBars(ui->customPlot_2->xAxis, ui->customPlot_2->yAxis);
  trackPointer2->setWidth(0.025);
  trackPointer2->setPen(QPen(QColor(Qt::white)));
  trackPointer2->setBrush(QBrush(QBrush(QColor(Qt::white))));

  QColor color1 = QColor(Qt::red);
  color1.setAlpha(50);
  QColor color2 = QColor(Qt::white);
  color2.setAlpha(20);

  barLoopStart_2 = new QCPBars(ui->customPlot_2->xAxis, ui->customPlot_2->yAxis);
  barLoopStart_2->setWidth(0.025);
  barLoopStart_2->setPen(QPen(QColor(color2)));
  barLoopStart_2->setBrush(QBrush(QBrush(QColor(color2))));

  barLoopEnd_2 = new QCPBars(ui->customPlot_2->xAxis, ui->customPlot_2->yAxis);
  barLoopEnd_2->setWidth(0.025);
  barLoopEnd_2->setPen(QPen(QColor(color2)));
  barLoopEnd_2->setBrush(QBrush(QBrush(QColor(color2))));

  QColor colors[4];
  colors[0] = QColor(Qt::red);
  colors[1] = QColor(Qt::green);
  colors[2] = QColor(Qt::blue);
  colors[3] = QColor(Qt::yellow);
  for(int i = 0; i < 4; i++) 
  {
      returnBar2[i] = new QCPBars(ui->customPlot_2->xAxis, ui->customPlot_2->yAxis);
      returnBar2[i]->setWidth(0.025);
      colors[i].setAlpha(150);
      returnBar2[i]->setPen(QPen(QColor(colors[i])));
      returnBar2[i]->setBrush(QBrush(QBrush(colors[i])));
      returnBar2[i]->setVisible(false);
  }

  customPlot->graph(1)->setPen(QPen(Qt::NoPen));
  customPlot->graph(1)->setBrush(QBrush(QBrush(color1)));
  customPlot->graph(2)->setPen(QPen(Qt::NoPen));
  customPlot->graph(2)->setBrush(QBrush(QBrush(color1)));

  customPlot->yAxis->setRange(-100000, 100000);
    customPlot->yAxis->setVisible(false);
    customPlot->yAxis2->setVisible(false);
    customPlot->xAxis2->setVisible(false);
    customPlot->xAxis->setVisible(false);

    x2.push_back(0);
    x2.push_back(0);
    y1.push_back(100000);
    y1.push_back(-100000);
    yStart_2.push_back(100000);
    yStart_2.push_back(-100000);
    xStart_2.push_back(0);
    xStart_2.push_back(0);
    yEnd_2.push_back(100000);
    yEnd_2.push_back(-100000);
    xEnd_2.push_back(0);
    xEnd_2.push_back(0);
    y2fill.push_back(100000);
    y2fill.push_back(-100000);
    x2fill.push_back(0);
    x2fill.push_back(0);
    yReturn2.push_back(100000);
    yReturn2.push_back(-100000);
    xReturn2.push_back(0);
    xReturn2.push_back(0);

    ui->customPlot->graph(1)->setVisible(false);
    ui->customPlot->graph(2)->setVisible(false);

  connect(&dataTimer2, SIGNAL(timeout()), this, SLOT(bracketDataSlot2()));

  dataTimer2.start(100);
}
//-------------------------------------------------------
void MainWindow::bracketDataSlot2()
{

  if(soundProc->panel2.audioReady)
  {
      ui->customPlot_2->xAxis->setRange(0, soundProc->panel2.audioLength);
      int n = soundProc->panel2.audioLength * (48000 * 2) + 1;

      static QVector<double> xII, yII;
      static int prevII = 0;
      if(soundProc->panel2.loadAudioInterruption)
      {
          prevII = 0;
          xII.clear();
          yII.clear();
          ui->customPlot_2->graph(0)->setData(xII, yII);
          ui->customPlot_2->replot();
          soundProc->panel2.loadAudioInterruption = false;
      }
      quint64 actPos = prevII * 3000;

  for (int i=1; actPos<n/sizeof(qint16) && i < 50; i++, prevII++)
    {
      lock.lockForWrite();
      int size = soundProc->panel2.channel1->size()/sizeof(qint16);
      if(actPos > size) {
          lock.unlock();
          break;
      }

      xII.push_back(actPos/48000.);

      yII.push_back(*(reinterpret_cast<qint16*>(soundProc->panel2.channel1->data())+actPos)
              +*(reinterpret_cast<qint16*>(soundProc->panel2.channel2->data())+actPos));
      lock.unlock();

      actPos += 3000;
      if(actPos % 96 == 0)
      {
        ui->customPlot_2->graph(0)->setData(xII, yII);
        ui->customPlot_2->replot();
      }
  }

  ui->customPlot_2->replot();

  if(actPos>=n/sizeof(qint16))
  {
      soundProc->panel2.audioReady = false;
      prevII = 0;
      xII.clear();
      yII.clear();
  }
  }

  for(int i = 0; i < 4; i++) 
  {
      if(soundProc->panel2.flags[i]) 
      {
          xReturn2[0] = xReturn2[1] = soundProc->panel2.flagPos[i]/48000;
          returnBar2[i]->setData(xReturn2, yReturn2);
          returnBar2[i]->setVisible(true);
          ui->customPlot_2->replot();
      }
      else 
      {
          returnBar2[i]->setVisible(false);
      }
  }

  if(soundProc->panel2.isLoopStartSet)
  {
      xStart_2[0]=soundProc->panel2.loopingStart/48000;
      xStart_2[1]=soundProc->panel2.loopingStart/48000;
      barLoopStart_2->setData(xStart_2, yStart_2);
      barLoopStart_2->setVisible(true);
      ui->customPlot_2->replot();
  }

  if(soundProc->panel2.isLoopEndSet)
  {
      xEnd_2[0]=soundProc->panel2.loopingEnd/48000;
      xEnd_2[1]=soundProc->panel2.loopingEnd/48000;
      barLoopEnd_2->setData(xEnd_2, yEnd_2);
  }

  if(soundProc->panel2.isLoopingSet)
  {
      QColor color1;

      if(soundProc->panel2.isLoopingActive)
      {
          color1 = QColor(Qt::red);
          color1.setAlpha(50);
      }
      else
      {
          color1 = QColor(Qt::white);
          color1.setAlpha(50);
      }
      
      barLoopStart_2->setPen(QPen(QColor(color1)));
      barLoopEnd_2->setPen(QPen(QColor(color1)));
      ui->customPlot_2->graph(1)->setBrush(QBrush(QBrush(color1)));
      ui->customPlot_2->graph(2)->setBrush(QBrush(QBrush(color1)));
      barLoopStart_2->setVisible(false);
      barLoopEnd_2->setVisible(false);
      double start =  xStart_2[0]=soundProc->panel2.loopingStart/48000;
      double end =  xEnd_2[0]=soundProc->panel2.loopingEnd/48000;
        x2fill[0] = (start);
        y2fill[0] = (100000);
        x2fill[1]=(end);
        y2fill[1] = (100000);
        ui->customPlot_2->graph(1)->setData(x2fill,y2fill);

        x2fill[0] = start;
        y2fill[0] = -100000;
        x2fill[1] = end;
        y2fill[1] = -100000;
        ui->customPlot_2->graph(2)->setData(x2fill,y2fill);

        ui->customPlot_2->graph(1)->setVisible(true);
        ui->customPlot_2->graph(2)->setVisible(true);
        ui->customPlot_2->replot();
  }
  else
  {
      ui->customPlot_2->graph(1)->setVisible(false);
      ui->customPlot_2->graph(2)->setVisible(false);
      barLoopEnd_2->setVisible(false);
      ui->customPlot_2->replot();
  }

  if(soundProc->panel2.duration > 0) 
  {
        x2[0] = soundProc->panel2.actPos/48000;
        x2[1]=  soundProc->panel2.actPos/48000;
        trackPointer2->setData(x2, y1);
        ui->customPlot_2->replot();
  }
}
//-------------------------------------------------------
void MainWindow::graphClicked2(QCPAbstractPlottable *plottable, int dataIndex)
{
  double czas = plottable->interface1D()->dataMainKey(dataIndex);  //TO JEST OS X - sekundy
  if(czas*48000 < soundProc->panel2.loopingStart || czas*48000 > soundProc->panel2.loopingEnd)
      soundProc->panel2.isLoopingActive = false;
  soundProc->panel2.actPos = czas*48000;
  soundProc->panel2.realPosition = czas*48000;
  x2[0] = czas;
  x2[1] = czas;
  trackPointer2->setData(x2, y1);
  ui->customPlot_2->replot();

}
//-------------------------------------------------------
void MainWindow::mouseMove2(QMouseEvent *event)
{
    if(event->buttons() == Qt::LeftButton)
    {
        double position = (event->x()-15) * soundProc->panel2.rewindParam/soundProc->panel2.audioLength;
        if(position*48000 < soundProc->panel2.loopingStart || position*48000 > soundProc->panel2.loopingEnd)
            soundProc->panel2.isLoopingActive = false;

     if(position < 0)
     {
         x2[0] = 0;
         x2[1] = 0;
         soundProc->panel2.actPos = 0;
         soundProc->panel2.realPosition = 0;
     }
     else if(position > soundProc->panel2.audioLength)
     {
         x2[0] = soundProc->panel2.audioLength-0.1;
         x2[1] = soundProc->panel2.audioLength-0.1;
         soundProc->panel2.actPos = soundProc->panel2.audioLength-0.1;
         soundProc->panel2.realPosition = soundProc->panel2.audioLength-0.1;
     }
     else
     {
         x2[0] = position;
         x2[1] = position;
         soundProc->panel2.actPos = position*48000;
         soundProc->panel2.realPosition = position*48000;
     }
     trackPointer2->setData(x2, y1);
     ui->customPlot_2->replot();
    }
}
//-------------------------------------------------------
//select audio for panel 1
void MainWindow::selectAudio() {
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open file"), "/home/", tr("Music Files (*.mp3 *.wav)"), 0, QFileDialog::DontUseNativeDialog);

    if(filename == "") return;

    ui->laudio->setText(filename.split('/').last());
    emit loadAudio(filename);
}
//------------------------------------------------------------
//select audio for panel 2
void MainWindow::selectAudio2() {
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open file"), "/home/", tr("Music Files (*.mp3)"), 0, QFileDialog::DontUseNativeDialog);

    if(filename == "") return;

    ui->laudio_2->setText(filename.split('/').last());
    emit loadAudio2(filename);
}
//------------------------------------------------------------
//save cross fader change action
void MainWindow::crossFaderChange(int value) {
    soundProc->crossFader = value;
    if(soundProc->panel1.audioReady)
        emit soundProc->panel1.writeToFile(soundProc->Actions::cross, soundProc->panel1.actPos,value);
    else if(soundProc->panel2.audioReady)
        emit soundProc->panel2.writeToFile(soundProc->Actions::cross, soundProc->panel2.actPos,value);
}
//-------------------------------------------------------
//save actions to file
void MainWindow::saveAction()
{
     QString fileName = QFileDialog::getSaveFileName(this,
         tr("Save action"), "/home", tr("save(*.acn)"), 0, QFileDialog::DontUseNativeDialog);

     if(fileName == "") return;

     emit saveActionToFile(fileName);

}
//-------------------------------------------------------
//load actions from file
void MainWindow::loadAction()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open action"), "/home", tr("Action Files (*.acn)"), 0, QFileDialog::DontUseNativeDialog);

    if(fileName == "") return;

    emit loadActionFromFile(fileName);
}
//functions to change sliders according to recorded acitons
void MainWindow::lowChange(int value)
{
    ui->sLow->setSliderPosition(value);
    ui->sLow->setValue(value);
}
//-------------------------------------------------------
void MainWindow::medChange(int value)
{
    ui->sMedium->setSliderPosition(value);
    ui->sMedium->setValue(value);
}
//-------------------------------------------------------
void MainWindow::highChange(int value)
{
    ui->sHigh->setSliderPosition(value);
    ui->sHigh->setValue(value);
}
//-------------------------------------------------------
void MainWindow::lowChange2(int value)
{
    ui->sLow_2->setSliderPosition(value);
    ui->sLow_2->setValue(value);
}
//-------------------------------------------------------
void MainWindow::medChange2(int value)
{
    ui->sMedium_2->setSliderPosition(value);
    ui->sMedium_2->setValue(value);
}
//-------------------------------------------------------
void MainWindow::highChange2(int value)
{
    ui->sHigh_2->setSliderPosition(value);
    ui->sHigh_2->setValue(value);
}
//-------------------------------------------------------
void MainWindow::crossChanger(int value)
{
    ui->sCrossfader->setSliderPosition(value);
    ui->sCrossfader->setValue(value);
}
//------------------------------------------------------------
//rotate disc 1
void MainWindow::rotate(float angle) {
    discImg[0]->setRotation(angle);
}
//------------------------------------------------------------
//rotate disc 2
void MainWindow::rotate2(float angle) {
    discImg[1]->setRotation(angle);
}
//------------------------------------------------------------
//set location and export recorded music file
void MainWindow::onExport() {

    //choosing the location of the saved audio file
    QString selectedFilter;
    QString filename = QFileDialog::getSaveFileName(this,
        tr("Export"), "/home", tr("audio(*.wav)"),&selectedFilter, QFileDialog::DontUseNativeDialog);

    if(filename.count() != 0){
        filename += ".wav";

        emit startExport(filename);
    }

}
//------------------------------------------------------------
MainWindow::~MainWindow()
{
    th_soundProc.quit();
    th_soundProc.wait();

    delete discImg[0];
    delete discImg[1];
    delete disc[0];
    delete disc[1];

    delete ui;
}
