#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QtDebug>
#include <QMessageBox>

#include "tileSources/CompositeTileSource.h"
#include "tileSources/OSMTileSource.h"
#include "guts/CompositeTileSourceConfigurationWidget.h"
#include "CircleObject.h"
#include "UAVParametersWidget.h"
#include "SensorParametersWidget.h"
#include "PlanningControlWidget.h"
#include "Planner.h"
#include "PlanningWizard.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Setup MapGraphicsScene and View
    _scene = new MapGraphicsScene(this);
    _view = new MapGraphicsView(_scene,this);
    this->setCentralWidget(_view);

    //Setup tile sources for the MapGraphicsView
    QSharedPointer<CompositeTileSource> composite(new CompositeTileSource());
    QSharedPointer<MapTileSource> osm(new OSMTileSource());
    QSharedPointer<MapTileSource> mqSat(new OSMTileSource(OSMTileSource::MapQuestAerialTiles));
    composite->addSourceBottom(osm,0.75);
    composite->addSourceBottom(mqSat);
    _view->setTileSource(composite);

    //Zoom into BYU campus by default
    QPointF place(-111.649253,40.249707);
    _view->setZoomLevel(15);
    _view->centerOn(place);

    //Provide our "map layers" dock widget with the composite tile source to be configured
    this->ui->mapLayersWidget->setComposite(composite);

    //When the user clicks "start planning" on the control widget, we want to start planning
    connect(this->ui->planningControlWidget,
            SIGNAL(planningStartRequested(qreal)),
            this,
            SLOT(handlePlanningControlStart(qreal)));

    connect(this->ui->planningControlWidget,
            SIGNAL(planningClearRequested()),
            this,
            SLOT(handlePlanningControlReset()));

    //When the user users the palette to request adding an end point
    connect(this->ui->paletteWidget,
            SIGNAL(addEndPointRequested()),
            this,
            SLOT(handleEndPointAddRequested()));

    //When the user users the palette to request adding a start point
    connect(this->ui->paletteWidget,
            SIGNAL(addStartPointRequested()),
            this,
            SLOT(handleStartPointAddRequested()));

    //Spawn a helpful wizard!
    /*
    PlanningWizard * wizard = new PlanningWizard(this);
    wizard->show();
    */

    //Maximize ourselves
    this->showMaximized();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//private slot
void MainWindow::handlePlanningControlStart(qreal desiredFitness)
{
    if (!_problem.isReady())
    {
        QMessageBox::information(this,
                                 "Cannot Plan - Problem is incomplete",
                                 "Planning cannot begin until you provide enough information");
        this->ui->planningControlWidget->setIsPlanningRunning(false);
        return;
    }

    Planner planner;
    Individual result = planner.plan(&_problem, desiredFitness);

    QList<QPointF> geo = result.generateGeoPoints(_problem.startingPos());

    foreach(QPointF pos, geo)
    {
        CircleObject * circle = new CircleObject(3.0,false,Qt::yellow);
        _scene->addObject(circle);
        circle->setPos(pos);
        _pathObjects.insert(circle);
    }

    this->ui->planningControlWidget->setIsPlanningRunning(false);
}

//private slot
void MainWindow::handlePlanningControlReset()
{
    foreach(MapGraphicsObject * obj, _pathObjects)
    {
        obj->deleteLater();
    }
    _pathObjects.clear();
}

//private slot
void MainWindow::handleStartPointAddRequested()
{
    if (_startPositionMarker.isNull())
    {
        _startPositionMarker = new CircleObject(6.0,true,Qt::green);
        _scene->addObject(_startPositionMarker);
        connect(_startPositionMarker.data(),
                SIGNAL(posChanged()),
                this,
                SLOT(handleStartPositionMarkerPosChanged()));
    }
    _startPositionMarker->setPos(_view->center());
}

//private slot
void MainWindow::handleEndPointAddRequested()
{
    if (_endPositionMarker.isNull())
    {
        _endPositionMarker = new CircleObject(6.0,true,Qt::red);
        _scene->addObject(_endPositionMarker);
        connect(_endPositionMarker.data(),
                SIGNAL(posChanged()),
                this,
                SLOT(handleEndPositionMarkerPosChanged()));
    }
    _endPositionMarker->setPos(_view->center());
}

//private slot
void MainWindow::handleStartPositionMarkerPosChanged()
{
    _problem.setStartingPos(_startPositionMarker->pos());
}

//private slot
void MainWindow::handleEndPositionMarkerPosChanged()
{
    _problem.setEndingPos(_endPositionMarker->pos());
}