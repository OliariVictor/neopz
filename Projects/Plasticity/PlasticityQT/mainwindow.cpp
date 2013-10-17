#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "initialpointdock.h"
#include "ui_initialpointdock.h"
#include "globalconfig.h"
#include <iostream>
#include <limits>
using namespace std;

#include <QMainWindow>
#include <QHash>
#include <QString>
#include <QListWidgetItem>
#include <QtGui>
#include <QSignalMapper>
#include <QMouseEvent>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_engine.h>
#include <qwt_symbol.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_marker.h>
#include <qwt_counter.h>
#include <qwt_plot.h>
#include "plot.h"
#include "simulation.h"
#include "canvaspicker.h"

#include "simulation.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(tr("Plasticity"));

//    ui->dockWidget_2->setGeometry(ui->dockWidget_2->geometry().x(),ui->dockWidget_2->geometry().y(), 500, 500);
//    ui->dockWidgetContents_2->setGeometry(ui->dockWidgetContents_2->geometry().x(),ui->dockWidgetContents_2->geometry().y(), 500, 500);
//    ui->centralWidget->setGeometry(ui->centralWidget->geometry().x(),ui->centralWidget->geometry().y(), 5, 5);
//    ui->dockWidget_2->setMinimumWidth(0);

    // Creating objects and linking them to their pointers
    this->FilesList = new QHash <int, TXT > ;

    //////////////////////////////// Interface stuff ////////////////////////////////

    // Test 1
    ui->Plot_1->setTitle("TEST 01");
    connect(ui->Plot_1->canvas_picker, SIGNAL(mouseLeftClicked(Plot *)),
            this, SLOT(clickedOnPlot(Plot *)));
    connect(ui->Plot_1->canvas_picker, SIGNAL(mouseDoubleClicked(Plot *)),
            this, SLOT(fullscreenOnPlot(Plot *)));
    connect(ui->Plot_1, SIGNAL(AxisChanged_signal (Plot*, QString)),
            this, SLOT(ChangePlotAxis (Plot*,QString)));
    // Initial Plot where graphs will be ploted
    currentPlot = ui->Plot_1;
    ui->Plot_1->setHighlighted(true);
    ui->Plot_1->canvas()->setCursor(Qt::CrossCursor);
    // Test 2
    ui->Plot_2->setTitle("TEST 02");
    connect(ui->Plot_2->canvas_picker, SIGNAL(mouseLeftClicked(Plot *)),
            this, SLOT(clickedOnPlot(Plot *)));
    connect(ui->Plot_2->canvas_picker, SIGNAL(mouseDoubleClicked(Plot *)),
            this, SLOT(fullscreenOnPlot(Plot *)));
    connect(ui->Plot_2, SIGNAL(AxisChanged_signal (Plot*, QString)),
            this, SLOT(ChangePlotAxis(Plot*,QString)));
    // Test 3
    ui->Plot_3->setTitle("TEST 03");
    connect(ui->Plot_3->canvas_picker, SIGNAL(mouseLeftClicked(Plot *)),
            this, SLOT(clickedOnPlot(Plot *)));
    connect(ui->Plot_3->canvas_picker, SIGNAL(mouseDoubleClicked(Plot *)),
            this, SLOT(fullscreenOnPlot(Plot *)));
    connect(ui->Plot_3, SIGNAL(AxisChanged_signal (Plot*, QString)),
            this, SLOT(ChangePlotAxis (Plot*,QString)));
    // Test 4
    ui->Plot_4->setTitle("TEST 04");
    connect(ui->Plot_4->canvas_picker, SIGNAL(mouseLeftClicked(Plot *)),
            this, SLOT(clickedOnPlot(Plot *)));
    connect(ui->Plot_4->canvas_picker, SIGNAL(mouseDoubleClicked(Plot *)),
            this, SLOT(fullscreenOnPlot(Plot *)));
    connect(ui->Plot_4, SIGNAL(AxisChanged_signal (Plot*, QString)),
            this, SLOT(ChangePlotAxis (Plot*,QString)));

    on_actionZoom_toggled(false);

    // TXT List Context menu
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(ShowListContextMenu(const QPoint&)));

    QSettings settings("myapp.ini", QSettings::IniFormat);
    settings.setValue("editor/wrapMargin", 68);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_listWidget_clicked(const QModelIndex &index)
{
    int listIndex = index.row();
    int indexCurve = ui->listWidget->item(listIndex)->data(5).toInt();
    int checkStatus = ui->listWidget->item(listIndex)->checkState();

    if (checkStatus == 2) {
        currentPlot->createCurve(indexCurve, this->FilesList->value(indexCurve).X, this->FilesList->value(indexCurve).Y, checkStatus);
    }
    else
    {
        currentPlot->deleteCurve(indexCurve);
    }
}

void MainWindow::fullscreenOnPlot(Plot *plotTmp)
{
    if (ui->Plot_1->isHidden() || ui->Plot_2->isHidden() || ui->Plot_3->isHidden() || ui->Plot_4->isHidden()) {
        ui->Plot_1->show();
        ui->Plot_2->show();
        ui->Plot_3->show();
        ui->Plot_4->show();
    }
    else {
        ui->Plot_1->hide();
        ui->Plot_2->hide();
        ui->Plot_3->hide();
        ui->Plot_4->hide();

        plotTmp->show();
    }
}

void MainWindow::clickedOnPlot(Plot *plotTmp)
{
    reloadCurvesList (plotTmp);

    //set plotTmp to highlighted
    ui->Plot_1->setHighlighted(false);
    ui->Plot_2->setHighlighted(false);
    ui->Plot_3->setHighlighted(false);
    ui->Plot_4->setHighlighted(false);
    plotTmp->setHighlighted(true);

    //set plotTmp cursor to cross
    ui->Plot_1->canvas()->setCursor(Qt::PointingHandCursor);
    ui->Plot_2->canvas()->setCursor(Qt::PointingHandCursor);
    ui->Plot_3->canvas()->setCursor(Qt::PointingHandCursor);
    ui->Plot_4->canvas()->setCursor(Qt::PointingHandCursor);
    plotTmp->canvas()->setCursor(Qt::CrossCursor);
}

void MainWindow::reloadCurvesList(Plot *plotTmp)
{
    currentPlot = plotTmp;

    // repopulating listWidgetQwtPlot
    int list_size = ui->listWidget->count();
    for (int i=0; i < list_size; i++) {
        QListWidgetItem *item = ui->listWidget->item(i);
        int item_id = item->data(5).toInt();
        item->setCheckState(Qt::CheckState(plotTmp->CurvesList->value(item_id).chk_status));
    }
}

void MainWindow::on_actionShowGraphList_triggered()
{
    ui->dockWidget->show();
}

void MainWindow::cutCurve(int indexCurves, int indexStartPoint, int indexEndPoint) {
    qDebug() << "CUTCurve MAIN idxcurve = " << indexCurves << " Start / end pts: " << indexStartPoint << " / " << indexEndPoint ;

    int size_tmp =this->FilesList->value(indexCurves).time->size();
    qDebug() << "Size = " << size_tmp;
    this->FilesList->value(indexCurves).time->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
    this->FilesList->value(indexCurves).time->remove(0,indexStartPoint);

    if (this->FilesList->value(indexCurves).defAxial->size() == size_tmp) {
        this->FilesList->value(indexCurves).defAxial->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
        this->FilesList->value(indexCurves).defAxial->remove(0,indexStartPoint);
    }
    if (this->FilesList->value(indexCurves).defLateral->size() == size_tmp) {
        this->FilesList->value(indexCurves).defLateral->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
        this->FilesList->value(indexCurves).defLateral->remove(0,indexStartPoint);
    }
    if (this->FilesList->value(indexCurves).defVol->size() == size_tmp) {
        this->FilesList->value(indexCurves).defVol->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
        this->FilesList->value(indexCurves).defVol->remove(0,indexStartPoint);
    }
    if (this->FilesList->value(indexCurves).sigmaAxialDesv->size() == size_tmp) {
        this->FilesList->value(indexCurves).sigmaAxialDesv->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
        this->FilesList->value(indexCurves).sigmaAxialDesv->remove(0,indexStartPoint);
    }
    if (this->FilesList->value(indexCurves).sigmaAxialTotal->size() == size_tmp) {
        this->FilesList->value(indexCurves).sigmaAxialTotal->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
        this->FilesList->value(indexCurves).sigmaAxialTotal->remove(0,indexStartPoint);
    }
    if (this->FilesList->value(indexCurves).sigmaConf->size() == size_tmp) {
        this->FilesList->value(indexCurves).sigmaConf->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
        this->FilesList->value(indexCurves).sigmaConf->remove(0,indexStartPoint);
    }
    if (this->FilesList->value(indexCurves).sigmaLateral->size() == size_tmp) {
        this->FilesList->value(indexCurves).sigmaLateral->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
        this->FilesList->value(indexCurves).sigmaLateral->remove(0,indexStartPoint);
    }
    if (this->FilesList->value(indexCurves).sigmaVol->size() == size_tmp) {
        this->FilesList->value(indexCurves).sigmaVol->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
        this->FilesList->value(indexCurves).sigmaVol->remove(0,indexStartPoint);
    }
    if (this->FilesList->value(indexCurves).I1->size() == size_tmp) {
        this->FilesList->value(indexCurves).I1->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
        this->FilesList->value(indexCurves).I1->remove(0,indexStartPoint);
    }
    if (this->FilesList->value(indexCurves).SQRTJ2->size() == size_tmp) {
        this->FilesList->value(indexCurves).SQRTJ2->remove(indexEndPoint+1,(size_tmp - 1) - indexEndPoint);
        this->FilesList->value(indexCurves).SQRTJ2->remove(0,indexStartPoint);
    }
}

void MainWindow::updateCurve(int indexCurves, int indexStartPoint, int indexEndPoint) {
    qDebug() << "UpdateCurve MAIN";
    ui->Plot_1->updateCurve( indexCurves, indexStartPoint, indexEndPoint );
    ui->Plot_2->updateCurve( indexCurves, indexStartPoint, indexEndPoint );
    ui->Plot_3->updateCurve( indexCurves, indexStartPoint, indexEndPoint );
    ui->Plot_4->updateCurve( indexCurves, indexStartPoint, indexEndPoint );
}

void MainWindow::ShowListContextMenu(const QPoint& pos)
{
    QListWidgetItem *listItem = ui->listWidget->itemAt(pos);
    if (!listItem) return;

    QPoint globalPos = ui->listWidget->mapToGlobal(pos); // for most widgets
    // QPoint globalPos = myWidget->viewport()->mapToGlobal(pos); // for QAbstractScrollArea and derived classes you would use:

    QMenu myMenu;
    QAction *aSaveAs = myMenu.addAction("Save curve as...");
    QAction *aSelectPoint = myMenu.addAction("Select initial point");
    QAction *aDeleteFile = myMenu.addAction("Unload file");
    if (listItem->checkState() == 2)
        aSelectPoint->setEnabled(1);
    else
        aSelectPoint->setDisabled(1);

    QAction* selectedItem = myMenu.exec(globalPos);

    if (aSaveAs == selectedItem) {

        QFileDialog Save_File(this);

        QString fileName = Save_File.getSaveFileName(this, "Save file as...",
                                                     ".","Text files (*.txt);; All files (*)");

        if (fileName == "") {
            qDebug() << "Error: file cannot be created.";
            return;
        }

        QFile curve_file;
        curve_file.setFileName (fileName);

        if (!curve_file.open(QIODevice::ReadWrite))
        {
            qDebug() << "Error: No filename specified.";
            return;
        }

        QTextStream *out_curve = new QTextStream (&curve_file);

        int indexCurve = ui->listWidget->item(ui->listWidget->indexAt(pos).row())->data(5).toInt();
        for (int i=0; i<this->FilesList->value(indexCurve).X->size(); i++) {
            *out_curve << this->FilesList->value(indexCurve).X->value(i) << " " << this->FilesList->value(indexCurve).Y->value(i) << endl;
        }

        //Atualizando entrada na tabela de arquivos / ListWidget
        TXT arquivo_1 = this->FilesList->take(indexCurve);
        QFileInfo infofile = QFileInfo (fileName);
        arquivo_1.name = infofile.baseName()+"."+infofile.suffix();

        this->FilesList->insert(indexCurve, arquivo_1);

        listItem->setText(arquivo_1.name);

        curve_file.close();
    }

    if (aSelectPoint == selectedItem) {

        int indexCurve = ui->listWidget->item(ui->listWidget->indexAt(pos).row())->data(5).toInt();

        // VERIFICAR COMO NAO CRIAR 2 DIALOGOS PARA MESMA CURVA
        initialpointdock *selectpointdock = new initialpointdock(this);

        // connecting signals -> slots to show/hide symbols curve depending on dock visibility
        connect(selectpointdock, SIGNAL(showSymbCurve(int)),
                currentPlot, SLOT(showSymbCurve(int)));
        connect(selectpointdock, SIGNAL(hideSymbCurve(int)),
                currentPlot, SLOT(hideSymbCurve(int)));
        // connecting signal/slot to perform curve cut
        connect(selectpointdock, SIGNAL(cutCurve(int,int,int)),
                currentPlot, SLOT(cutCurve(int,int,int)));
        // this one is responsable for cutting all vectors of points (sigmas* and defs*) from the target curve
        connect(selectpointdock, SIGNAL(cutCurve(int,int,int)),
                this, SLOT(cutCurve(int,int,int)));
        // connecting signal/slot to perform curve redraw with new data
        connect(selectpointdock, SIGNAL(cutCurve(int,int,int)),
                this, SLOT(updateCurve(int,int,int)));
        // connecting signals -> slots to sync information when points are changed
        connect(selectpointdock, SIGNAL(SymbPointChanged(int,int,Plot::pointType)),
                currentPlot->canvas_picker, SLOT(setSymbPoint(int,int,Plot::pointType)));
        connect(currentPlot->canvas_picker, SIGNAL(SymbPointChanged(int,int,Plot::pointType)),
                selectpointdock, SLOT(setSymbPoint(int,int,Plot::pointType)));

        selectpointdock->setIndexCurve(indexCurve);
        selectpointdock->setXdata(currentPlot->CurvesList->value(indexCurve).X);
        selectpointdock->setYdata(currentPlot->CurvesList->value(indexCurve).Y);
        int indexStartPoint = currentPlot->getSymbIndex(indexCurve, Plot::startPoint);
        int indexEndPoint   = currentPlot->getSymbIndex(indexCurve, Plot::endPoint);
        qDebug() << "aSelectPoint == selectedItem: [indexCurve] indexStartPoint / indexEndPoint" << "[" << indexCurve << "] " << indexStartPoint << " " << indexEndPoint ;
        selectpointdock->setSlidersMinimum(indexStartPoint);
        selectpointdock->setSlidersMaximum(indexEndPoint);
        selectpointdock->setSymbPoint(indexStartPoint, indexCurve, Plot::startPoint);
        selectpointdock->setSymbPoint(indexEndPoint, indexCurve, Plot::endPoint);
        selectpointdock->setWindowTitle(listItem->text());
        selectpointdock->setFloating(1);
        selectpointdock->setGeometry(300,250,selectpointdock->width(),selectpointdock->height());
        selectpointdock->show();

    }

    if (aDeleteFile == selectedItem) {

        int indexCurve = ui->listWidget->indexAt(pos).row();
        QListWidgetItem *tmp_item =  ui->listWidget->takeItem( indexCurve );

        indexCurve = -1;
        indexCurve = tmp_item->data(5).toInt();
        qDebug() <<"INDEX CURVE!@%#$@!#$%$" <<indexCurve;
        currentPlot->deleteCurve(indexCurve);
        this->FilesList->remove(indexCurve);

        delete tmp_item;

    }
}

void MainWindow::on_actionZoom_toggled(bool on)
{
    ui->Plot_1->panner->setEnabled(on);
//    ui->Plot_1->canvas()->setCursor();
    ui->Plot_1->zoomer->setEnabled(on);

    ui->Plot_2->panner->setEnabled(on);
    ui->Plot_2->zoomer->setEnabled(on);

    ui->Plot_3->panner->setEnabled(on);
    ui->Plot_3->zoomer->setEnabled(on);

    ui->Plot_4->panner->setEnabled(on);
    ui->Plot_4->zoomer->setEnabled(on);
}

void MainWindow::on_actionOpenFile_triggered()
{
    QFileDialog Open_File(this);
    QStringList fileNames = Open_File.getOpenFileNames(this, "Select one or more files to open",
                                                 ".","Text files (*.txt);; All files (*)");

    foreach (QString fileName, fileNames) {

        if (fileName == "") {
            qDebug() << "Error: No filename specified.";
            return;
        }

        QFile curve_file;
        curve_file.setFileName (fileName);

        if (!curve_file.open(QFile::ReadOnly | QFile::Text))
        {
            qDebug() << "Error: file not loaded.";
            return;
        }

        double valueX, valueY;
        QVector<double> *Xvalues;
        QVector<double> *Yvalues;
        QVector<double> *time = new QVector<double>();
        QVector<double> *sigmaAxialTotal = new QVector<double>();
        QVector<double> *sigmaAxialDesv = new QVector<double>();
        QVector<double> *sigmaConf = new QVector<double>();
        QVector<double> *defAxial = new QVector<double>();
        QVector<double> *defLateral = new QVector<double>();
        QVector<double> *defVol = new QVector<double>();
        QVector<double> *sigmaVol = new QVector<double>();
        QVector<double> *sigmaLateral;// = new QVector<double>();
        QVector<double> *I1 = new QVector<double>();
        QVector<double> *SQRTJ2 = new QVector<double>();

        int countLines = 0;
        double Xsmallest=numeric_limits<double>::max(), Xbiggest=numeric_limits<double>::min(),
               Ysmallest=numeric_limits<double>::max(), Ybiggest=numeric_limits<double>::min();
        int typeTest = testTypes(All);

        QString line = curve_file.readLine(); // header line, ignoring it
        QStringList strings = line.split( QRegExp("[\\t]+") );

        if (strings.size() == 8) {
            qDebug() << "UCS";
            typeTest = testTypes(UCS); //0;
        }

        if (strings.size() == 12) {
            qDebug() << "Triaxial";
            typeTest = testTypes(Triaxial); //1;
        }

        while (!curve_file.atEnd()) {
            line = curve_file.readLine();
            line.chop(1); // to remove \n of the end

            strings = line.split( QRegExp("[\\t]+") );

            if (strings.size() < 2) continue;

            if (typeTest == testTypes(UCS)) { // TESTE UCS !!!!!!!!!!!!!!!!!!

                // Variaveis
                // Tempo	SigAxialDesv	DefAxial	DefLateral	DefVolume

                // Variaveis do arquivo txt
                time->insert( countLines, strings.value(0).toDouble() );
                sigmaAxialDesv->insert( countLines, strings.value(1).toDouble() );
                defAxial->insert( countLines, strings.value(3).toDouble() );
                defLateral->insert( countLines, strings.value(5).toDouble() );
                defVol->insert( countLines, strings.value(7).toDouble() );

                // Atribuicoes
                sigmaAxialTotal->insert( countLines, 1.5 * strings.value(1).toDouble());
                sigmaLateral=sigmaAxialTotal;
                sigmaVol=sigmaAxialTotal;
                I1 = sigmaAxialTotal;
                SQRTJ2->insert( countLines, sqrt((1/3.)*((1.5*strings.value(1).toDouble())*(1.5*strings.value(1).toDouble())))); // J2 = 1/3*Sax^2

                qDebug() << "SIGMA AXIAL = " <<sigmaAxialTotal->value(countLines) <<endl <<"I1 = " <<I1->value(countLines) <<endl <<"SQRTJ2 = " <<SQRTJ2->value(countLines) <<endl <<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" <<endl;

                // Variaveis utilizadas apenas para ajustar escala (logo abaixo)
                valueY = strings.value(1).toDouble(); //sigmaAxialTotal
                valueX = strings.value(3).toDouble();

                // Coordenadas Default (Tensao axial x Deformacao axial)
                Xvalues = defAxial;
                Yvalues = sigmaAxialTotal;
            }
            else if (typeTest == testTypes(Triaxial)) // TESTE TRIAXIAL !!!!!!!!!!!!!!!!!!!
            {

                // Variaveis
                // Tempo	**SigAxialTotal**	SigAxialDesv	**SigConf**	DefAxial	DefLateral	DefVolume

                // Variaveis do arquivo txt
                time->insert( countLines, strings.value(0).toDouble() );
                sigmaAxialTotal->insert( countLines, strings.value(1).toDouble() ); // Triaxial ONLY
                sigmaAxialDesv->insert( countLines, strings.value(3).toDouble() );
                sigmaConf->insert( countLines, strings.value(5).toDouble() );       // Triaxial ONLY
                defAxial->insert( countLines, strings.value(7).toDouble() );
                defLateral->insert( countLines, strings.value(9).toDouble() );
                defVol->insert( countLines, strings.value(11).toDouble() );

                // Atribuicoes
                sigmaVol->insert( countLines, (strings.value(1).toDouble()+(strings.value(5).toDouble()) * 2 )); // Svol = Sax + 2* Sconf
                sigmaLateral = sigmaConf;
                I1->insert( countLines, (strings.value(1).toDouble()+(2*strings.value(5).toDouble()))); // I1 = Sax + 2*Sconf
                SQRTJ2->insert( countLines, sqrt((strings.value(3).toDouble())*(strings.value(3).toDouble())*(1/3.)) ); // J2 = 1/3*Sdesv^2

                qDebug() << "SIGMA AXIAL = " <<sigmaAxialTotal->value(countLines) <<endl  << "SIGMA CONF = " <<sigmaConf->value(countLines) <<endl <<"I1 = " <<I1->value(countLines) <<endl <<"SQRTJ2 = " <<SQRTJ2->value(countLines) <<endl <<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" <<endl;

                // Variaveis utilizadas apenas para ajustar escala (logo abaixo)
                valueY = strings.value(1).toDouble(); //sigmaAxialTotal
                valueX = strings.value(7).toDouble(); //defAxial

                // Coordenadas Default (TensaoxDeformacao)
                Xvalues = defAxial;
                Yvalues = sigmaAxialTotal;
          }

            if (valueX < Xsmallest) Xsmallest = valueX;
            if (valueX > Xbiggest) Xbiggest = valueX;
            if (valueY < Ysmallest) Ysmallest = valueY;
            if (valueY > Ybiggest) Ybiggest = valueY;

//            qDebug() << "--->" << strings.size();
//            foreach (QString field, strings) {
//                qDebug() << QVariant(field).toDouble();
//            }
//            qDebug() << "<---";
            countLines ++ ;
        }

        qDebug() << "Count X / Y: " << Xvalues->size() << "/" << Yvalues->size() << " count: " << countLines;

        curve_file.close();

        //Criando entrada na tabela de arquivos
        TXT arquivoTXT;
        QFileInfo infofile = QFileInfo (fileName);
        arquivoTXT.name = infofile.baseName()+"."+infofile.suffix();
        arquivoTXT.X = Xvalues;
        arquivoTXT.Y = Yvalues;
        arquivoTXT.time = time;
        arquivoTXT.sigmaAxialDesv = sigmaAxialDesv;
        arquivoTXT.sigmaAxialTotal = sigmaAxialTotal;
        arquivoTXT.sigmaConf = sigmaConf;
        arquivoTXT.defAxial = defAxial;
        arquivoTXT.defLateral = defLateral;
        arquivoTXT.defVol = defVol;
        arquivoTXT.sigmaLateral = sigmaLateral;
        arquivoTXT.sigmaVol = sigmaVol;
        arquivoTXT.testType = testTypes(typeTest);
        arquivoTXT.I1 = I1;
        arquivoTXT.SQRTJ2 = SQRTJ2;


        //int position = this->FilesList->size();
        int position = 0;
        if (this->FilesList->size() != 0) {
            int last_key = this->FilesList->keys().last();
            position = last_key+1;
        }
        this->FilesList->insert(position, arquivoTXT);
        //Criando entrada na listWidget
        QListWidgetItem *item;
        item = new QListWidgetItem();
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setText(this->FilesList->value(position).name);
        item->setData(5,position); //item "ID"
        ui->listWidget->addItem(item);

    }
}

void MainWindow::on_actionExportPlot_triggered()
{
    currentPlot->exportPlot();
}

void MainWindow::ChangePlotAxis(Plot* plot_ptr,QString PlotAxis)
{
    qDebug() << "CHANGE PLOT AXIS!!!!!!!" << plot_ptr << PlotAxis;
    //xvalues_ptr mudar pelo XCoords(TEXTO da nova coordenada)
    //OU
    //Chamar o updatecurves para todas as curvas - parametros:
    // index_curve: foreach para percorrer toda a lista de curvas do plot
    // startpoint e endpoint: pegar da struct curves
    //OU (ULTIMO CASO)
    //plot_ptr->delete all curves :vai apagar as curvas da lista de curvas do plot
    //elas vao continuar na listinha do lado, mas talvez com check - FORCAR O UNCHECK!


    // mudar os eixos de plotagem (colunas a serem lidas no arquivo)
    // mudar os nomes dos eixos

    //OBS EIXOS: VER O OPENFILE EU ACHO, TA COLOCANDO TENSIOXDEFORMATION QUANDO ABRE UM ARQUIVO

    foreach (int i, plot_ptr->CurvesList->keys())
    {
        // Sigma ax x Epsilon ax
        QString Coords1 = QChar (0x03C3);
        Coords1.append("ax x ");
        Coords1.append(QChar (0x03B5));
        Coords1.append("ax");

        if (PlotAxis == Coords1) {

            CURVE d_curve_tmp = plot_ptr->CurvesList->take(i);

            // COLUNAS LIDAS NO ARQUIVO
            d_curve_tmp.X = this->FilesList->value(i).defAxial; //OK!
            d_curve_tmp.Y = this->FilesList->value(i).sigmaAxialTotal; //OK!

            d_curve_tmp.X2 = d_curve_tmp.X3 = d_curve_tmp.Y2 = d_curve_tmp.Y3 = NULL;

            //re-inserting new value
            plot_ptr->CurvesList->insert(i, d_curve_tmp);

            //naming axis
            QString Title1Y = QChar (0x03C3);
            Title1Y.append("ax (MPa)");
            plot_ptr->setAxisTitle(QwtPlot::yLeft, Title1Y);
            QString Title1X = QChar (0x03B5);
            Title1X.append("ax (%)");
            plot_ptr->setAxisTitle(QwtPlot::xBottom, Title1X);

            // update curve with new data
            this->updateCurve(i, 0, d_curve_tmp.X->size()-1);
        }

        // Sigma rad x Epsilon rad
        QString Coords2 = QChar (0x03C3);
        Coords2.append("rad x ");
        Coords2.append(QChar (0x03B5));
        Coords2.append("rad");

        if (PlotAxis == Coords2) {

            CURVE d_curve_tmp = plot_ptr->CurvesList->take(i);

            // COLUNAS LIDAS NO ARQUIVO
            d_curve_tmp.X = this->FilesList->value(i).defLateral; //OK!
            d_curve_tmp.Y = this->FilesList->value(i).sigmaLateral; //OK!

            d_curve_tmp.X2 = d_curve_tmp.X3 = d_curve_tmp.Y2 = d_curve_tmp.Y3 = NULL;

//            qDebug () << d_curve_tmp.X->toList();

            //re-inserting new value
            plot_ptr->CurvesList->insert(i, d_curve_tmp);

            //naming axis
            QString Title2Y = QChar (0x03C3);
            Title2Y.append("rad (MPa)");
            plot_ptr->setAxisTitle(QwtPlot::yLeft, Title2Y);
            QString Title2X = QChar (0x03B5);
            Title2X.append("rad (%)");
            plot_ptr->setAxisTitle(QwtPlot::xBottom, Title2X);
            // update curve with new data
            this->updateCurve(i, 0, d_curve_tmp.X->size()-1);
        }

        // Sigma vol x Epsilon vol
        QString Coords3 = QChar (0x03C3);
        Coords3.append("vol x ");
        Coords3.append(QChar (0x03B5));
        Coords3.append("vol");

        if (PlotAxis == Coords3) {

            CURVE d_curve_tmp = plot_ptr->CurvesList->take(i);

            d_curve_tmp.X = this->FilesList->value(i).defVol; //OK!
            d_curve_tmp.Y = this->FilesList->value(i).sigmaVol; //OK!

            d_curve_tmp.X2 = d_curve_tmp.X3 = d_curve_tmp.Y2 = d_curve_tmp.Y3 = NULL;

            //re-inserting new value
            plot_ptr->CurvesList->insert(i, d_curve_tmp);

            //naming axis
            QString Title3Y = QChar (0x03C3);
            Title3Y.append("vol (MPa)");
            plot_ptr->setAxisTitle(QwtPlot::yLeft, Title3Y);
            QString Title3X = QChar (0x03B5);
            Title3X.append("vol (%)");
            plot_ptr->setAxisTitle(QwtPlot::xBottom, Title3X);
            // update curve with new data
            this->updateCurve(i, 0, d_curve_tmp.X->size()-1);
        }

        // Sigma ax x Epsilon ax Epsilon vol Epsilon rad
        QString Coords4 = QChar (0x03C3);
        Coords4.append("ax x ");
        Coords4.append(QChar (0x03B5));
        Coords4.append("ax ");
        Coords4.append(QChar (0x03B5));
        Coords4.append("vol ");
        Coords4.append(QChar (0x03B5));
        Coords4.append("rad");

        if (PlotAxis == Coords4) {

            CURVE d_curve_tmp = plot_ptr->CurvesList->take(i);

            d_curve_tmp.X = this->FilesList->value(i).defAxial; //OK!
            d_curve_tmp.Y = this->FilesList->value(i).sigmaAxialTotal; //OK!

            // IMPLEMENTAR ACIMA, PRA PEGAR COMO X E Y OS VALORES DE SIGMA A E EPSILONS A,V,L

            d_curve_tmp.X2 = this->FilesList->value(i).defLateral; //OK!
            d_curve_tmp.Y2 = this->FilesList->value(i).sigmaAxialTotal; //OK!
            d_curve_tmp.X3 = this->FilesList->value(i).defVol; //OK!
            d_curve_tmp.Y3 = this->FilesList->value(i).sigmaAxialTotal; //OK!

            //re-inserting new value
            plot_ptr->CurvesList->insert(i, d_curve_tmp);

            //naming axis
            QString Title4Y = QChar (0x03C3);
            Title4Y.append("ax (MPa)");
            plot_ptr->setAxisTitle(QwtPlot::yLeft, Title4Y);
            QString Title4X = QChar (0x03C3);
            Title4X.append("ax x ");
            Title4X.append(QChar (0x03B5));
            Title4X.append("ax ");
            Title4X.append(QChar (0x03B5));
            Title4X.append("vol ");
            Title4X.append(QChar (0x03B5));
            Title4X.append("rad (%)");
            plot_ptr->setAxisTitle(QwtPlot::xBottom, Title4X);
            // update curve with new data
            this->updateCurve(i, 0, d_curve_tmp.X->size()-1);
        }

        // I1 x sqrt J2
        QString Coords5 = "I1 x ";
        Coords5.append(QChar (0x221A));
        Coords5.append("J2");

        if (PlotAxis == Coords5) {

            CURVE d_curve_tmp = plot_ptr->CurvesList->take(i);

            d_curve_tmp.X = this->FilesList->value(i).SQRTJ2;
            d_curve_tmp.Y = this->FilesList->value(i).I1;
            // IMPLEMENTAR ACIMA, PRA PEGAR COMO X E Y OS VALORES DE I1 E RAIZ DE J2

            d_curve_tmp.X2 = d_curve_tmp.X3 = d_curve_tmp.Y2 = d_curve_tmp.Y3 = NULL;

            //re-inserting new value
            plot_ptr->CurvesList->insert(i, d_curve_tmp);

            //naming axis
            QString Title5Y = "I1";
            plot_ptr->setAxisTitle(QwtPlot::yLeft, Title5Y);
            QString Title5X = QChar (0x221A);
            Title5X.append("J2");
            plot_ptr->setAxisTitle(QwtPlot::xBottom, Title5X);
            // update curve with new data
            this->updateCurve(i, 0, d_curve_tmp.X->size()-1);
        }

//        // Epsilon v x sqrt J2Epsilon
//        QString Coords6 = QChar (0x03B5);
//        Coords6.append("v x ");
//        Coords6.append(QChar (0x221A));
//        Coords6.append("J2");
//        Coords6.append(QChar (0x03B5));

//        if (PlotAxis == Coords6) {

//            CURVE d_curve_tmp = plot_ptr->CurvesList->take(i);

//            d_curve_tmp.X = this->FilesList->value(i).defAxial;
//            d_curve_tmp.Y = this->FilesList->value(i).sigmaAxialDesv;
//            // IMPLEMENTAR ACIMA, PRA PEGAR COMO X E Y OS VALORES DE EPSILON V E RAIZ DE J2 EPSILON

//            d_curve_tmp.X2 = d_curve_tmp.X3 = d_curve_tmp.Y2 = d_curve_tmp.Y3 = NULL;

//            //re-inserting new value
//            plot_ptr->CurvesList->insert(i, d_curve_tmp);

//            //naming axis
//            QString Title6Y = QChar (0x03B5);
//            Title6Y.append("v");
//            plot_ptr->setAxisTitle(QwtPlot::yLeft, Title6Y);
//            QString Title6X = QChar (0x221A);
//            Title6X.append("J2");
//            Title6X.append(QChar (0x03B5));
//            plot_ptr->setAxisTitle(QwtPlot::xBottom, Title6X);
//            // update curve with new data
//            this->updateCurve(i, 0, d_curve_tmp.X->size()-1);
//        }
    }

}

void MainWindow::on_actionShow_Parameter_List_triggered()
{
    ui->dockWidget_2->show();
}

void MainWindow::on_pushButton_clicked()
{
    // EXECUTAR SIMULATION!!!!!!!!!!!!!!!!

    TPZPlasticityTest simulacao;
    // PERCORRER LISTA DE ARQUIVOS E RODAR PASSO ABAIXO P TODOS ELES!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // ver se funciona apos cortar a curva
    // CRIAR FLAG P/ SABERMOS QUAL EH SIMULADO E QUAL EH DADO REAL NA LISTA DE ARQUIVOS ??
    simulacao.LoadInputStrainStress(this->FilesList->value(0).sigmaAxialTotal, this->FilesList->value(0).sigmaLateral, this->FilesList->value(0).defAxial, this->FilesList->value(0).defLateral);

    TPZSandlerDimaggio<SANDLERDIMAGGIOSTEP2> sandler;
    REAL inttol = 1.e-4;
    sandler.SetIntegrTol(inttol);
    simulacao.SetSandlerDimaggio(sandler);

    simulacao.SetUpSimulation(ui->poisson_counter->value(), ui->young_counter->value(), ui->A_counter->value(),
                              ui->B_counter->value(), ui->C_counter->value(), ui->R_counter->value(),
                              ui->D_counter->value(), ui->W_counter->value());


    simulacao.PerformSimulation();

//    simulacao.PrintResults();

    {
    //Criando entrada na tabela de arquivos
    TXT arquivoTXT;
    arquivoTXT.name = "Simulacao Stress";
    arquivoTXT.X = new QVector<double> (simulacao.get_Strain_X());
    arquivoTXT.Y = new QVector<double> (simulacao.get_Stress_X());
    arquivoTXT.sigmaAxialDesv = new QVector<double> (simulacao.get_Stress_X());
    arquivoTXT.sigmaAxialTotal = new QVector<double> (simulacao.get_Stress_X());
    arquivoTXT.sigmaConf = new QVector<double> (simulacao.get_Stress_X());
    arquivoTXT.sigmaLateral = new QVector<double> (simulacao.get_Stress_X());
    arquivoTXT.sigmaVol = new QVector<double> (simulacao.get_Stress_Y());

    arquivoTXT.defAxial = new QVector<double> (simulacao.get_Strain_X());
    arquivoTXT.defLateral = new QVector<double> (simulacao.get_Strain_X());
    arquivoTXT.defVol = new QVector<double> (simulacao.get_Strain_Y());

    //int position = this->FilesList->size();
    int position = 0;
    if (this->FilesList->size() != 0) {
        int last_key = this->FilesList->keys().last();
        position = last_key+1;
    }
    this->FilesList->insert(position, arquivoTXT);
    //Criando entrada na listWidget
    QListWidgetItem *item;
    item = new QListWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);
    item->setText(this->FilesList->value(position).name);
    item->setData(5,position); //item "ID"
    ui->listWidget->addItem(item);
    }

    qDebug() << "END Simulation...";

}
