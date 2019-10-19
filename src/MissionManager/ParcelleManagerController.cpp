#include "ParcelleManagerController.h"

#include "DataManager/DbManager.h"
#include "Admin/GeoportailLink.h"
#include "ComplexMissionItem.h"
#include "SurveyComplexItem.h"
#include <QSqlTableModel>
#include <QNetworkReply>
#include "MissionController.h"
#include "QGCApplication.h"
#include "Admin/List_file.h"
#include "ShapeFileHelper.h"


extern QString username;
extern DbManager *db;
extern List_file *speedParam;

ParcelleManagerController::ParcelleManagerController() {
    geoportailParcelle = new GeoportailLink();
    connect(geoportailParcelle, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestReply(QNetworkReply*)));
}

ParcelleManagerController::~ParcelleManagerController()
{

}


void ParcelleManagerController::deleteParcelle(SqlParcelleModel *model, QList<int> indexes) {
    for(int i=0; i<indexes.size();i++) {
        qDebug() << "Removing " << indexes[i];
        QFile file (model->record(i).value("parcelleFile").toString());
        file.remove();
        model->removeRow(indexes[i]);
    }
    model->submitAll();
}


void ParcelleManagerController::addToMission(SqlParcelleModel *model,MissionController *missionController, QList<int> indexes) {
    qDebug() << "in userSpace::addToMission";


    QMap<QString, double> *KmlParcelleList= new QMap<QString, double>() ;

    for(int i=0; i<indexes.size();i++)
    {
        QString file = model->record(i).value("parcelleFile").toString();
        double speed = speedParam->at(model->record(i).value("speed").toInt()).toDouble();
        qDebug() << i;
        KmlParcelleList->insert(file, speed); // ici il faudra mettre le path
    }
    missionController->insertComplexMissionFromDialog(*KmlParcelleList);
    this->deleteLater();
}

void ParcelleManagerController::addParcelle(SqlParcelleModel *model, QString ilotNumber, QString file, QString type, int speed) {
    _file = file;

    qDebug() << "in addParcelle";

    QSqlRecord newRecord = model->record();
    newRecord.setValue("owner", QVariant(username));
    newRecord.setValue("parcelleFile", QVariant(file));
    newRecord.setValue("type", QVariant(type));
    newRecord.setValue("speed",QVariant(speed));

    /*-1 is set to indicate that it will be added to the last row*/
    if(model->insertRecord(-1, newRecord)) {
        qDebug()<<"successful insertion" << newRecord.value("owner") << "was its owner";
        model->submitAll();
    }
    qDebug() << "addParcelle";

    this->requestParcelle(ilotNumber);
}

void ParcelleManagerController::requestParcelle(QString NbIlot) {

    QString req = "request=GetCapabilities&SERVICE=WFS&VERSION=2.0.0&request=GetFeature&typeName=RPG.2016:parcelles_graphiques&outputFormat=kml&srsname=EPSG:2154&FILTER=%3CFilter%20xmlns=%22http://www.opengis.net/fes/2.0%22%3E%20%3CPropertyIsEqualTo%3E%20%3CValueReference%3Eid_parcel%3C/ValueReference%3E%20%3CLiteral%3E"+ NbIlot +"%3C/Literal%3E%20%3C/PropertyIsEqualTo%3E%20%3C/Filter%3E";
    geoportailParcelle->requestGeo(req);
    return;
}

void ParcelleManagerController::requestReply(QNetworkReply *reply) {
    qDebug() << "requestReply";
    if (reply->error()) {
        qDebug() << reply->errorString();
        emit downloadEnded(false);
        return;
    }

    QString answer = reply->readAll();

    // dans une reponse normale, il n'y a qu'un polygon de decrit.
    if (answer.count("<Polygon>") == 1) {
        ShapeFileHelper::savePolygonFromGeoportail(_file, answer);
        //add to DB!
        emit downloadEnded(true);
        return;
    }
    qDebug() << "ERROR no awnser or too many";
    emit downloadEnded(false);
    return;
}

void ParcelleManagerController::modifyParcelle(SqlParcelleModel *model, int index, QString owner, QString parcelleFile, QString type, int speed) {
    //TODO : Modify parcelle
}

