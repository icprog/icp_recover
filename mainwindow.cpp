#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>


#include "./src_libmodbus/modbus.h"


#define FLAG 0
#define LINE 1
#define YEAR 3
#define MONTH 4
#define DAY 5
#define HOUR 6
#define FIRSTDATA 7


struct uzel
{
    QString ObjectName;
    QString IP_addr;
    QString OdbcName;
    QString TableName;
    QString type;  //"vosn","1uvr2nord","1uvr2nord1mm","4uvr","gaz"
    QString text;
    QString dt;
} uzels[]=
{


    "LELAKI","172.16.223.2","fire_lelaki","LELAKI_SOU","vosn","","",

    "GGPZ_GNED","172.16.57.69","fire_ggpz_gned","GGPZ_GNED_SOU","vosn","","",
    "GGPZ_LELAKI_VHOD","172.16.57.70","fire_ggpz_lelaki_vhod","GGPZ_LELAKI_VHOD_SOU","vosn","","",
//    "GGPZ_LELAKI_VIHOD","172.16.57.72","fire_ggpz_lelaki_vihod","GGPZ_LELAKI_VIHOD_SOU","vosn","","",

    "GGPZ_Z_OS","172.16.57.75","fire_ggpz_z_os","GGPZ_Z_OS_SOU","vosn","","",
    "GGPZ_Z_UPS","172.16.57.76","fire_ggpz_z_ups","GGPZ_Z_UPS_SOU","vosn","","",
    "GGPZ_Z_KSU","172.16.57.78","fire_ggpz_z_ksu","GGPZ_Z_KSU_SOU","1uvr2nord1mm","","",
    "GZU1_GNED",  "172.16.57.20","fire_gzu1_gned","GZU1_GNED_SOU","vosn","","",
    "GGPZ_4VOV","172.16.57.74","fire_ggpz_4vov","GGPZ_4VOV_SOU","4uvr","","",
    "YAROSHIV","172.16.48.100","fire_yaroshiv","YAROSHIV_SOU","vosn","","",
    "TALAL_Z_YAROSHIV","172.16.48.102","fire_talal_z_yaroshiv","TALAL_Z_YAROSHIV_SOU","vosn","","",
    "GGPZ_GAZ","172.16.57.72","fire_ggpz_gaz","GGPZ_GAZ","gaz","","",
};
//==========================================================================================
QString GetNextName()
{
static int counter=1;
QString res;
res.sprintf("%u",counter);
counter=(counter+1) % 1000000;
return res;
}
//==========================================================================================

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //connect(ui->pushButtonCheckHour,SIGNAL(clicked()),this,SLOT(PushButtonCheckHour()));
    connect(ui->pushButtonCheckDay,SIGNAL(clicked()),this,SLOT(PushButtonCheckDay()));
    //connect(ui->pushButtonRecoveryHour,SIGNAL(clicked()),this,SLOT(PushButtonRecoveryHour()));
    connect(ui->pushButtonRecoveryDay,SIGNAL(clicked()),this,SLOT(PushButtonRecoveryDay()));
    connect(ui->pushButtonClose,SIGNAL(clicked()),this,SLOT(close()));

    for(int i=0;i<sizeof(uzels)/sizeof(uzel);++i)
    {
        ui->listWidget->addItem(uzels[i].ObjectName+" " + uzels[i].IP_addr+" ("+ uzels[i].type + ")" + " -> ODBC:" + uzels[i].OdbcName+", TABLE:"+uzels[i].TableName);
    }



}
//==========================================================================================
MainWindow::~MainWindow()
{
    delete ui;
}
//==========================================================================================
int CheckHour(uzel *p_uzel)
{

    QString connectionName=GetNextName();

    int ret=-1;




        {  // start of the block where the db object lives

            QSqlDatabase db = QSqlDatabase::addDatabase("QODBC",connectionName);
                  db.setDatabaseName(p_uzel->OdbcName);
                  db.setUserName("sysdba");//user);
                  db.setPassword("784523");//pass);
            if (db.open())
            {

                QSqlQuery sqlQuery(db);
                sqlQuery.exec(QString("SELECT * FROM ")+p_uzel->TableName+" WHERE DT='"+p_uzel->dt+"'");

                if (sqlQuery.lastError().isValid())
                {
                    //QMessageBox::critical(NULL, QObject::tr("Database Error"), sqlQuery.lastError().text());
                    p_uzel->text="db SELECT error!!!";
                    ret=-2;
                }
                else
                {
                    ret=1;
                    QSqlRecord rec = sqlQuery.record();

                    int rowcount=0;
                    while (sqlQuery.next())
                    {
                        rowcount++;
                    }
                    if (rowcount>0)
                    {
                        p_uzel->text="OK:found "+QString::number(rowcount)+ " records";
                        ret=rowcount;
                    }
                    else
                    {
                        p_uzel->text="No records found!!!";
                        ret=0;
                    }
                }



            }
            else
            {
                //QMessageBox::critical(NULL, QObject::tr("Database Error"), db.lastError().text());
                p_uzel->text="db open error!!!";
                ret=-1;
            }

            db.close();
        } // end of the block where the db object lives, it will be destroyed here

        QSqlDatabase::removeDatabase(connectionName);



return ret;

}
//==========================================================================================
int RecoveryHourVosn(uzel *p_uzel, uint year_,uint month_, uint day_,uint hour_)
{

    int ret=0;

    modbus_t *mb;
    uint16_t tab_reg[100];

    uint16_t flag;
    uint16_t line;

    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    float GustPlastVod;
    float GustSepNaf;
    float GustGazuSU;
    float Gazovmist;
    float ObjomVilnGazu;
    float DiametrTrubopr;
    float KoeffZ;

    //masstotal
    float mass_total;

    //средних, делить на cycle_count
    float d_rFlowMM_avg;
    float d_rTemp_avg;
    float d_rTisk_avg;
    float d_rObvodn_opos_avg;
    float d_rDens_avg;

    //накопленные
    float d_rMasRid_MM_SOU;
    float d_rMasNaf_MM_SOU;
    float d_rObjemRidRU_MM_SOU;
    float d_rObjemNafRU_MM_SOU;  //d_rObjemGazonasZnevodnNafRU_MM_SOU
    float d_rObjemNafSU_MM_SOU;

    //накопленные - added for PIP-VSN
    float d_rObvodn_avg; //added for PIP-VSN
    float d_rMasRid_MM_SOU_PIP;
    float d_rMasNaf_MM_SOU_PIP;
    float d_rObjemRidRU_MM_SOU_PIP;
    float d_rObjemNafRU_MM_SOU_PIP;   //d_rObjemGazonasZnevodnNafRU_MM_SOU_PIP
    float d_rObjemNafSU_MM_SOU_PIP;

    //added for NORD1

    float d_rFlowNORD1_avg;
    // NORD1+SOU
    float d_rObjemRid_NORD1_SOU;
    float d_rObjemRidRU_NORD1_SOU;
    float d_rMassNaf_NORD1_SOU;
    float d_rObjomNafSU_NORD1_SOU;
    float d_rObjomNafGnRU_NORD1_SOU;


    //added for NORD1+PIP
    float d_rObjemRid_NORD1_SOU_PIP;
    float d_rObjemRidRU_NORD1_SOU_PIP;
    float d_rMassNaf_NORD1_SOU_PIP;
    float d_rObjomNafSU_NORD1_SOU_PIP;
    float d_rObjomNafGnRU_NORD1_SOU_PIP;


    uint16_t alarmsCode;
    uint16_t alarmsTimeSec;
    uint16_t WhichKoeffSaved;


    mb = modbus_new_tcp(p_uzel->IP_addr.toStdString().c_str(), 502);

    if (modbus_connect(mb)!=0 )
    {
        //QMessageBox::information(this,"Test","Зв'язок з об'єктом відсутній!!!",QMessageBox::Ok);
        ret=-1;
        p_uzel->text="Error connect to modbus interface";
        //emit textchange(i, "No connection with source");
    }
    else //connect OK
    {
        modbus_set_slave(mb, 1);

        tab_reg[FLAG]=1;
        tab_reg[LINE]=1;
        tab_reg[YEAR]=year_;
        tab_reg[MONTH]=month_;
        tab_reg[DAY]=day_;
        tab_reg[HOUR]=hour_;

        if (modbus_write_registers(mb, 499, 7, tab_reg)==7)  //query data
        {
            Sleep(3000);

            int res=modbus_read_registers(mb, 499, 87, tab_reg);

            if (res!=87)
            {
                ret=-3;
                p_uzel->text="Error read from modbus";

            }
            else //read OK
            {

                flag=tab_reg[0];
                line=tab_reg[1];
                year=tab_reg[3];
                month=tab_reg[4];
                day=tab_reg[5];
                hour=tab_reg[6];

                if (flag==0 && line==1 && year==year_ && month==month_ && day==day_ && hour==hour_)
                {
                    GustPlastVod=modbus_get_float(&tab_reg[7]);
                    GustSepNaf=modbus_get_float(&tab_reg[9]);
                    GustGazuSU=modbus_get_float(&tab_reg[11]);
                    Gazovmist=modbus_get_float(&tab_reg[13]);
                    ObjomVilnGazu=modbus_get_float(&tab_reg[15]);
                    DiametrTrubopr=modbus_get_float(&tab_reg[17]);
                    KoeffZ=modbus_get_float(&tab_reg[19]);
                    //masstotal
                    mass_total=modbus_get_float(&tab_reg[21]);

                    //средних, делить на cycle_count
                    d_rFlowMM_avg=modbus_get_float(&tab_reg[23]);
                    d_rTemp_avg=modbus_get_float(&tab_reg[25]);
                    d_rTisk_avg=modbus_get_float(&tab_reg[27]);
                    d_rObvodn_opos_avg=modbus_get_float(&tab_reg[29]);
                    d_rDens_avg=modbus_get_float(&tab_reg[31]);

                    //накопленные
                    d_rMasRid_MM_SOU=modbus_get_float(&tab_reg[33]);
                    d_rMasNaf_MM_SOU=modbus_get_float(&tab_reg[35]);
                    d_rObjemRidRU_MM_SOU=modbus_get_float(&tab_reg[37]);
                    d_rObjemNafRU_MM_SOU=modbus_get_float(&tab_reg[39]);  //d_rObjemGazonasZnevodnNafRU_MM_SOU
                    d_rObjemNafSU_MM_SOU=modbus_get_float(&tab_reg[41]);

                    //накопленные - added for PIP-VSN
                    d_rObvodn_avg=modbus_get_float(&tab_reg[43]); //added for PIP-VSN
                    d_rMasRid_MM_SOU_PIP=modbus_get_float(&tab_reg[45]);
                    d_rMasNaf_MM_SOU_PIP=modbus_get_float(&tab_reg[47]);
                    d_rObjemRidRU_MM_SOU_PIP=modbus_get_float(&tab_reg[49]);
                    d_rObjemNafRU_MM_SOU_PIP=modbus_get_float(&tab_reg[51]);   //d_rObjemGazonasZnevodnNafRU_MM_SOU_PIP
                    d_rObjemNafSU_MM_SOU_PIP=modbus_get_float(&tab_reg[53]);

                    //added for NORD1

                    d_rFlowNORD1_avg=modbus_get_float(&tab_reg[55]);
                    // NORD1+SOU
                    d_rObjemRid_NORD1_SOU=modbus_get_float(&tab_reg[57]);
                    d_rObjemRidRU_NORD1_SOU=modbus_get_float(&tab_reg[59]);
                    d_rMassNaf_NORD1_SOU=modbus_get_float(&tab_reg[61]);
                    d_rObjomNafSU_NORD1_SOU=modbus_get_float(&tab_reg[63]);
                    d_rObjomNafGnRU_NORD1_SOU=modbus_get_float(&tab_reg[65]);


                    //added for NORD1+PIP
                    d_rObjemRid_NORD1_SOU_PIP=modbus_get_float(&tab_reg[67]);
                    d_rObjemRidRU_NORD1_SOU_PIP=modbus_get_float(&tab_reg[69]);
                    d_rMassNaf_NORD1_SOU_PIP=modbus_get_float(&tab_reg[71]);
                    d_rObjomNafSU_NORD1_SOU_PIP=modbus_get_float(&tab_reg[73]);
                    d_rObjomNafGnRU_NORD1_SOU_PIP=modbus_get_float(&tab_reg[75]);  //40373


                    //rezerv1 =modbus_get_float(&tab_reg[77]); 40377
                    //rezerv2 =modbus_get_float(&tab_reg[79]); 40379


                    alarmsCode=tab_reg[81];
                    alarmsTimeSec=tab_reg[82];
                    WhichKoeffSaved=tab_reg[83];


                    QString connectionName=GetNextName();

                        {  // start of the block where the db object lives

                            QSqlDatabase db = QSqlDatabase::addDatabase("QODBC",connectionName);
                                  db.setDatabaseName(p_uzel->OdbcName);
                                  db.setUserName("sysdba");//user);
                                  db.setPassword("784523");//pass);
                            if (db.open())
                            {

                                QSqlQuery sqlQuery(db);
                                QString query;

                                    query.sprintf(QString("INSERT INTO " + p_uzel->TableName + "(" +
                                                  "DT, TEMP, TISK, OBVODN_OPOS, DENS, OBVODN, " +
                                                  "GUSTPLASTVOD, GUSTSEPNAF, GUSTGAZUSU, GAZOVMIST, OBJOMVILNGAZU, DIAMETRTRUBOPR, KOEFFZ, "+
                                                  "FLOWMM, MASSARIDMM, OBJEMRIDRUMM, OBJEMNAFMM, MASSANAFMM, "+
                                                  "MASSARIDMM_PIP, OBJEMRIDRUMM_PIP, OBJEMNAFMM_PIP, MASSANAFMM_PIP, "+
                                                  "FLOWNORD1, OBJEMRIDNORD1, OBJEMNAFSUNORD1, MASSANAFNORD1, OBJEMRIDRUNORD1, "+
                                                  "OBJEMRIDNORD1_PIP, OBJEMRIDRUNORD1_PIP, MASSANAFNORD1_PIP, OBJEMNAFSUNORD1_PIP, "+    //, LEVEL_E2, OBJEM_E2, DELTA_E2)
                                                  "ALARMSCODE , ALARMSTIMESEC, WHICHKOEFFSAVED) "
                                                  "VALUES ("+
                                                  "'%i.%i.%i %i:00:00', %f, %f, %f, %f, %f, "+
                                                  "%f, %f, %f, %f, %f, %f, %f, " +
                                                  "%f, %f, %f, %f, %f, "
                                                  "%f, %f, %f, %f, "
                                                  "%f, %f, %f, %f, %f, " +
                                                  "%f, %f, %f, %f,"
                                                  "%i, %i, %i)").toStdString().c_str(),
                                                  day,month,year,hour,d_rTemp_avg,d_rTisk_avg,d_rObvodn_opos_avg,d_rDens_avg,d_rObvodn_avg,
                                                  GustPlastVod,GustSepNaf,GustGazuSU,Gazovmist,ObjomVilnGazu,DiametrTrubopr,KoeffZ,
                                                  d_rFlowMM_avg,d_rMasRid_MM_SOU,d_rObjemRidRU_MM_SOU,d_rObjemNafSU_MM_SOU,d_rMasNaf_MM_SOU,
                                                  d_rMasRid_MM_SOU_PIP,d_rObjemRidRU_MM_SOU_PIP,d_rObjemNafSU_MM_SOU_PIP,d_rMasNaf_MM_SOU_PIP,
                                                  d_rFlowNORD1_avg,d_rObjemRid_NORD1_SOU,d_rObjomNafSU_NORD1_SOU,d_rMassNaf_NORD1_SOU,d_rObjemRidRU_NORD1_SOU,
                                                  d_rObjemRid_NORD1_SOU_PIP,d_rObjemRidRU_NORD1_SOU_PIP,d_rMassNaf_NORD1_SOU_PIP,d_rObjomNafSU_NORD1_SOU_PIP,
                                                  alarmsCode, alarmsTimeSec, WhichKoeffSaved
                                                  );

                                                  //  emit insert(query);

                                    sqlQuery.exec(query);

                                    if (sqlQuery.lastError().isValid())
                                    {
                                        //QMessageBox::critical(NULL, QObject::tr("Database Error"), sqlQuery.lastError().text());
                                        //emit textchange(i,"ERROR: Database error on INSERT");
                                        ret=-5;
                                        p_uzel->text="ERROR: Database error on INSERT";
                                    }
                                    else
                                    {
                                        //QMessageBox::information(NULL, tr("Информация"), "Запись удалена успешно.");
                                        //emit textchange(i,"INSERT OK");
                                        ret=1;
                                        p_uzel->text="Data recover succesfully";
                                    }




                            }
                            else
                            {
                                //QMessageBox::critical(NULL, QObject::tr("Database Error"), db.lastError().text());
                                //emit insert(db.lastError().databaseText());
                                //emit insert(db.lastError().driverText());
                                //emit textchange(i,"ERROR: Database not open");
                                ret=-6;
                                p_uzel->text="ERROR: Database open error";
                            }

                            db.close();
                        } // end of the block where the db object lives, it will be destroyed here

                        QSqlDatabase::removeDatabase(connectionName);

                    //insert to DB end




                }
                else
                {
                    p_uzel->text="No data in PLC archive";
                    ret=-4;
                }

            }
        }
        else
        {
            ret=-2;
            p_uzel->text="Error write query data to modbus";
        }



    }

        modbus_close(mb);
        modbus_free(mb);

return ret;

}
//==========================================================================================
int RecoveryHour1Uvr2Nord(uzel *p_uzel, uint year_,uint month_, uint day_,uint hour_)
{

    int ret=0;

    modbus_t *mb;
    uint16_t tab_reg[100];

    uint16_t flag;
    uint16_t line;

    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    //uvr
    float m_rUvrVolFlow_avg;
    float m_rUvrVolTotal;
    float d_rUvrObjem;
    //NORD 1
    float d_rNORD1_VolFlow_avg;
    float d_rNORD1_Objem;
    //NORD 2
    float d_rNORD2_VolFlow_avg;
    float d_rNORD2_Objem;



    uint16_t alarmsCode;
    uint16_t alarmsTimeSec;
    uint16_t WhichKoeffSaved;


    mb = modbus_new_tcp(p_uzel->IP_addr.toStdString().c_str(), 502);

    if (modbus_connect(mb)!=0 )
    {
        //QMessageBox::information(this,"Test","Зв'язок з об'єктом відсутній!!!",QMessageBox::Ok);
        ret=-1;
        p_uzel->text="Error connect to modbus interface";
        //emit textchange(i, "No connection with source");
    }
    else //connect OK
    {
        modbus_set_slave(mb, 1);

        tab_reg[FLAG]=1;
        tab_reg[LINE]=1;
        tab_reg[YEAR]=year_;
        tab_reg[MONTH]=month_;
        tab_reg[DAY]=day_;
        tab_reg[HOUR]=hour_;

        if (modbus_write_registers(mb, 499, 7, tab_reg)==7)  //query data
        {
            Sleep(3000);

            int res=modbus_read_registers(mb, 499, 27, tab_reg);

            if (res!=27)
            {
                ret=-3;
                p_uzel->text="Error read from modbus";

            }
            else //read OK
            {

                flag=tab_reg[0];
                line=tab_reg[1];
                year=tab_reg[3];
                month=tab_reg[4];
                day=tab_reg[5];
                hour=tab_reg[6];

                if (flag==0 && line==1 && year==year_ && month==month_ && day==day_ && hour==hour_)
                {

                    //uvr
                    m_rUvrVolFlow_avg=modbus_get_float(&tab_reg[7]);;
                    m_rUvrVolTotal=modbus_get_float(&tab_reg[9]);;
                    d_rUvrObjem=modbus_get_float(&tab_reg[11]);;
                    //NORD 1
                    d_rNORD1_VolFlow_avg=modbus_get_float(&tab_reg[13]);;
                    d_rNORD1_Objem=modbus_get_float(&tab_reg[15]);;
                    //NORD 2
                    d_rNORD2_VolFlow_avg=modbus_get_float(&tab_reg[17]);;
                    d_rNORD2_Objem=modbus_get_float(&tab_reg[19]);;

                    alarmsCode=tab_reg[21];
                    alarmsTimeSec=tab_reg[22];
                    WhichKoeffSaved=tab_reg[23];


                    QString connectionName=GetNextName();

                        {  // start of the block where the db object lives

                            QSqlDatabase db = QSqlDatabase::addDatabase("QODBC",connectionName);
                                  db.setDatabaseName(p_uzel->OdbcName);
                                  db.setUserName("sysdba");//user);
                                  db.setPassword("784523");//pass);
                            if (db.open())
                            {

                                QSqlQuery sqlQuery(db);
                                QString query;

                                query.sprintf(QString("INSERT INTO " + p_uzel->TableName + "(" +
                                              "DT, UVRVOLFLOW, UVRVOLTOTAL, UVROBJEM, " +
                                              "NORD1_VOLFLOW, NORD1_OBJEM, NORD2_VOLFLOW, NORD2_OBJEM, "+
                                              "ALARMSCODE , ALARMSTIMESEC, WHICHKOEFFSAVED) "
                                              "VALUES ("+
                                              "'%i.%i.%i %i:00:00', %f, %f, %f, "+
                                              "%f, %f, %f, %f, " +
                                              "%i, %i, %i)").toStdString().c_str(),
                                              day,month,year,hour,m_rUvrVolFlow_avg,m_rUvrVolTotal,d_rUvrObjem,
                                              d_rNORD1_VolFlow_avg,d_rNORD1_Objem,d_rNORD2_VolFlow_avg,d_rNORD2_Objem,
                                              alarmsCode, alarmsTimeSec, WhichKoeffSaved
                                              );

                                                  //  emit insert(query);

                                    sqlQuery.exec(query);

                                    if (sqlQuery.lastError().isValid())
                                    {
                                        //QMessageBox::critical(NULL, QObject::tr("Database Error"), sqlQuery.lastError().text());
                                        //emit textchange(i,"ERROR: Database error on INSERT");
                                        ret=-5;
                                        p_uzel->text="ERROR: Database error on INSERT";
                                    }
                                    else
                                    {
                                        //QMessageBox::information(NULL, tr("Информация"), "Запись удалена успешно.");
                                        //emit textchange(i,"INSERT OK");
                                        ret=1;
                                        p_uzel->text="Data recover succesfully";
                                    }




                            }
                            else
                            {
                                //QMessageBox::critical(NULL, QObject::tr("Database Error"), db.lastError().text());
                                //emit insert(db.lastError().databaseText());
                                //emit insert(db.lastError().driverText());
                                //emit textchange(i,"ERROR: Database not open");
                                ret=-6;
                                p_uzel->text="ERROR: Database open error";
                            }

                            db.close();
                        } // end of the block where the db object lives, it will be destroyed here

                        QSqlDatabase::removeDatabase(connectionName);

                    //insert to DB end




                }
                else
                {
                    p_uzel->text="No data in PLC archive";
                    ret=-4;
                }

            }
        }
        else
        {
            ret=-2;
            p_uzel->text="Error write query data to modbus";
        }



    }

        modbus_close(mb);
        modbus_free(mb);

return ret;

}
//==========================================================================================
int RecoveryHour1Uvr2Nord1MM(uzel *p_uzel, uint year_,uint month_, uint day_,uint hour_)
{

    int ret=0;

    modbus_t *mb;
    uint16_t tab_reg[100];

    uint16_t flag;
    uint16_t line;

    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    //uvr
    float m_rUvrVolFlow_avg;
    float m_rUvrVolTotal;
    float d_rUvrObjem;
    //NORD 1
    float d_rNORD1_VolFlow_avg;
    float d_rNORD1_Objem;
    //NORD 2
    float d_rNORD2_VolFlow_avg;
    float d_rNORD2_Objem;
    //MM
    //masstotal
    float mass_total;
    float vol_total;
    float d_rMassFlow_avg;
    float d_rVolFlow_avg;
    float d_rDens_avg;
    float d_rObjemRidRU_MM;
    float d_rMassaRid_MM;


    uint16_t alarmsCode;
    uint16_t alarmsTimeSec;
    uint16_t WhichKoeffSaved;


    mb = modbus_new_tcp(p_uzel->IP_addr.toStdString().c_str(), 502);

    if (modbus_connect(mb)!=0 )
    {
        //QMessageBox::information(this,"Test","Зв'язок з об'єктом відсутній!!!",QMessageBox::Ok);
        ret=-1;
        p_uzel->text="Error connect to modbus interface";
        //emit textchange(i, "No connection with source");
    }
    else //connect OK
    {
        modbus_set_slave(mb, 1);

        tab_reg[FLAG]=1;
        tab_reg[LINE]=1;
        tab_reg[YEAR]=year_;
        tab_reg[MONTH]=month_;
        tab_reg[DAY]=day_;
        tab_reg[HOUR]=hour_;

        if (modbus_write_registers(mb, 499, 7, tab_reg)==7)  //query data
        {
            Sleep(3000);

            int res=modbus_read_registers(mb, 499, 27, tab_reg);

            if (res!=27)
            {
                ret=-3;
                p_uzel->text="Error read from modbus";

            }
            else //read OK
            {

                flag=tab_reg[0];
                line=tab_reg[1];
                year=tab_reg[3];
                month=tab_reg[4];
                day=tab_reg[5];
                hour=tab_reg[6];

                if (flag==0 && line==1 && year==year_ && month==month_ && day==day_ && hour==hour_)
                {

                    //uvr
                    m_rUvrVolFlow_avg=modbus_get_float(&tab_reg[7]);
                    m_rUvrVolTotal=modbus_get_float(&tab_reg[9]);
                    d_rUvrObjem=modbus_get_float(&tab_reg[11]);
                    //NORD 1
                    d_rNORD1_VolFlow_avg=modbus_get_float(&tab_reg[13]);
                    d_rNORD1_Objem=modbus_get_float(&tab_reg[15]);
                    //NORD 2
                    d_rNORD2_VolFlow_avg=modbus_get_float(&tab_reg[17]);
                    d_rNORD2_Objem=modbus_get_float(&tab_reg[19]);

                    //MM
                    mass_total=modbus_get_float(&tab_reg[21]);
                    vol_total=modbus_get_float(&tab_reg[23]);
                    d_rMassFlow_avg=modbus_get_float(&tab_reg[25]);
                    d_rVolFlow_avg=modbus_get_float(&tab_reg[27]);
                    d_rDens_avg=modbus_get_float(&tab_reg[29]);
                    d_rObjemRidRU_MM=modbus_get_float(&tab_reg[31]);
                    d_rMassaRid_MM=modbus_get_float(&tab_reg[33]);

                    alarmsCode=tab_reg[35];
                    alarmsTimeSec=tab_reg[36];
                    WhichKoeffSaved=tab_reg[37];


                    QString connectionName=GetNextName();

                        {  // start of the block where the db object lives

                            QSqlDatabase db = QSqlDatabase::addDatabase("QODBC",connectionName);
                                  db.setDatabaseName(p_uzel->OdbcName);
                                  db.setUserName("sysdba");//user);
                                  db.setPassword("784523");//pass);
                            if (db.open())
                            {

                                QSqlQuery sqlQuery(db);
                                QString query;

                                query.sprintf(QString("INSERT INTO " + p_uzel->TableName + "(" +
                                              "DT, UVRVOLFLOW, UVRVOLTOTAL, UVROBJEM, " +
                                              "NORD1_VOLFLOW, NORD1_OBJEM, NORD2_VOLFLOW, NORD2_OBJEM, "+
                                              "MM_MASSTOTAL, MM_VOLTOTAL, MM_MASSFLOW, MM_VOLFLOW, MM_DENS, MM_OBJEMRIDRU, MM_MASSARID, "+
                                              "ALARMSCODE , ALARMSTIMESEC, WHICHKOEFFSAVED) "
                                              "VALUES ("+
                                              "'%i.%i.%i %i:00:00', %f, %f, %f, "+
                                              "%f, %f, %f, %f, " +
                                              "%f, %f, %f, %f, %f, %f, %f, " +
                                              "%i, %i, %i)").toStdString().c_str(),
                                              day,month,year,hour,m_rUvrVolFlow_avg,m_rUvrVolTotal,d_rUvrObjem,
                                              d_rNORD1_VolFlow_avg,d_rNORD1_Objem,d_rNORD2_VolFlow_avg,d_rNORD2_Objem,
                                              mass_total,vol_total,d_rMassFlow_avg,d_rVolFlow_avg,d_rDens_avg,d_rObjemRidRU_MM,d_rMassaRid_MM,
                                              alarmsCode, alarmsTimeSec, WhichKoeffSaved
                                              );

                                                  //  emit insert(query);

                                    sqlQuery.exec(query);

                                    if (sqlQuery.lastError().isValid())
                                    {
                                        //QMessageBox::critical(NULL, QObject::tr("Database Error"), sqlQuery.lastError().text());
                                        //emit textchange(i,"ERROR: Database error on INSERT");
                                        ret=-5;
                                        p_uzel->text="ERROR: Database error on INSERT";
                                    }
                                    else
                                    {
                                        //QMessageBox::information(NULL, tr("Информация"), "Запись удалена успешно.");
                                        //emit textchange(i,"INSERT OK");
                                        ret=1;
                                        p_uzel->text="Data recover succesfully";
                                    }




                            }
                            else
                            {
                                //QMessageBox::critical(NULL, QObject::tr("Database Error"), db.lastError().text());
                                //emit insert(db.lastError().databaseText());
                                //emit insert(db.lastError().driverText());
                                //emit textchange(i,"ERROR: Database not open");
                                ret=-6;
                                p_uzel->text="ERROR: Database open error";
                            }

                            db.close();
                        } // end of the block where the db object lives, it will be destroyed here

                        QSqlDatabase::removeDatabase(connectionName);

                    //insert to DB end




                }
                else
                {
                    p_uzel->text="No data in PLC archive";
                    ret=-4;
                }

            }
        }
        else
        {
            ret=-2;
            p_uzel->text="Error write query data to modbus";
        }



    }

        modbus_close(mb);
        modbus_free(mb);

return ret;

}
//==========================================================================================
int RecoveryHour4Uvr(uzel *p_uzel, uint year_,uint month_, uint day_,uint hour_)
{
    int ret=0;

    modbus_t *mb;
    uint16_t tab_reg[100];

    uint16_t flag;
    uint16_t line;

    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    //uvr 1
    float m_rUvr1_VolFlow_avg;
    float m_rUvr1_VolTotal;
    float d_rUvr1_Objem;

    //uvr 2
    float m_rUvr2_VolFlow_avg;
    float m_rUvr2_VolTotal;
    float d_rUvr2_Objem;

    //uvr 3
    float m_rUvr3_VolFlow_avg;
    float m_rUvr3_VolTotal;
    float d_rUvr3_Objem;

    //uvr 4
    float m_rUvr4_VolFlow_avg;
    float m_rUvr4_VolTotal;
    float d_rUvr4_Objem;



    uint16_t alarmsCode;
    uint16_t alarmsTimeSec;
    uint16_t WhichKoeffSaved;


    mb = modbus_new_tcp(p_uzel->IP_addr.toStdString().c_str(), 502);

    if (modbus_connect(mb)!=0 )
    {
        //QMessageBox::information(this,"Test","Зв'язок з об'єктом відсутній!!!",QMessageBox::Ok);
        ret=-1;
        p_uzel->text="Error connect to modbus interface";
        //emit textchange(i, "No connection with source");
    }
    else //connect OK
    {
        modbus_set_slave(mb, 1);

        tab_reg[FLAG]=1;
        tab_reg[LINE]=1;
        tab_reg[YEAR]=year_;
        tab_reg[MONTH]=month_;
        tab_reg[DAY]=day_;
        tab_reg[HOUR]=hour_;

        if (modbus_write_registers(mb, 499, 7, tab_reg)==7)  //query data
        {
            Sleep(3000);

            int res=modbus_read_registers(mb, 499, 34, tab_reg);

            if (res!=34)
            {
                ret=-3;
                p_uzel->text="Error read from modbus";

            }
            else //read OK
            {

                flag=tab_reg[0];
                line=tab_reg[1];
                year=tab_reg[3];
                month=tab_reg[4];
                day=tab_reg[5];
                hour=tab_reg[6];

                if (flag==0 && line==1 && year==year_ && month==month_ && day==day_ && hour==hour_)
                {

                    //uvr 1
                    m_rUvr1_VolFlow_avg=modbus_get_float(&tab_reg[7]);;
                    m_rUvr1_VolTotal=modbus_get_float(&tab_reg[9]);;
                    d_rUvr1_Objem=modbus_get_float(&tab_reg[11]);;

                    //uvr 2
                    m_rUvr2_VolFlow_avg=modbus_get_float(&tab_reg[13]);;
                    m_rUvr2_VolTotal=modbus_get_float(&tab_reg[15]);;
                    d_rUvr2_Objem=modbus_get_float(&tab_reg[17]);;

                    //uvr 3
                    m_rUvr3_VolFlow_avg=modbus_get_float(&tab_reg[19]);;
                    m_rUvr3_VolTotal=modbus_get_float(&tab_reg[21]);;
                    d_rUvr3_Objem=modbus_get_float(&tab_reg[23]);;

                    //uvr 4
                    m_rUvr4_VolFlow_avg=modbus_get_float(&tab_reg[25]);;
                    m_rUvr4_VolTotal=modbus_get_float(&tab_reg[27]);;
                    d_rUvr4_Objem=modbus_get_float(&tab_reg[29]);;

                    alarmsCode=tab_reg[31];
                    alarmsTimeSec=tab_reg[32];
                    WhichKoeffSaved=tab_reg[33];


                    QString connectionName=GetNextName();

                        {  // start of the block where the db object lives

                            QSqlDatabase db = QSqlDatabase::addDatabase("QODBC",connectionName);
                                  db.setDatabaseName(p_uzel->OdbcName);
                                  db.setUserName("sysdba");//user);
                                  db.setPassword("784523");//pass);
                            if (db.open())
                            {

                                QSqlQuery sqlQuery(db);
                                QString query;

                                query.sprintf(QString("INSERT INTO " + p_uzel->TableName + "(" +
                                              "DT, UVR1_VOLFLOW, UVR1_VOLTOTAL, UVR1_OBJEM, " +
                                              "UVR2_VOLFLOW, UVR2_VOLTOTAL, UVR2_OBJEM, " +
                                              "UVR3_VOLFLOW, UVR3_VOLTOTAL, UVR3_OBJEM, " +
                                              "UVR4_VOLFLOW, UVR4_VOLTOTAL, UVR4_OBJEM, " +
                                              "ALARMSCODE , ALARMSTIMESEC, WHICHKOEFFSAVED) "
                                              "VALUES ("+
                                              "'%i.%i.%i %i:00:00', %f, %f, %f, "+
                                              "%f, %f, %f, " +
                                              "%f, %f, %f, " +
                                              "%f, %f, %f, " +
                                              "%i, %i, %i)").toStdString().c_str(),
                                              day,month,year,hour,m_rUvr1_VolFlow_avg,m_rUvr1_VolTotal,d_rUvr1_Objem,
                                              m_rUvr2_VolFlow_avg,m_rUvr2_VolTotal,d_rUvr2_Objem,
                                              m_rUvr3_VolFlow_avg,m_rUvr3_VolTotal,d_rUvr3_Objem,
                                              m_rUvr4_VolFlow_avg,m_rUvr4_VolTotal,d_rUvr4_Objem,
                                              alarmsCode, alarmsTimeSec, WhichKoeffSaved
                                              );

                                                  //  emit insert(query);

                                    sqlQuery.exec(query);

                                    if (sqlQuery.lastError().isValid())
                                    {
                                        //QMessageBox::critical(NULL, QObject::tr("Database Error"), sqlQuery.lastError().text());
                                        //emit textchange(i,"ERROR: Database error on INSERT");
                                        ret=-5;
                                        p_uzel->text="ERROR: Database error on INSERT";
                                    }
                                    else
                                    {
                                        //QMessageBox::information(NULL, tr("Информация"), "Запись удалена успешно.");
                                        //emit textchange(i,"INSERT OK");
                                        ret=1;
                                        p_uzel->text="Data recover succesfully";
                                    }




                            }
                            else
                            {
                                //QMessageBox::critical(NULL, QObject::tr("Database Error"), db.lastError().text());
                                //emit insert(db.lastError().databaseText());
                                //emit insert(db.lastError().driverText());
                                //emit textchange(i,"ERROR: Database not open");
                                ret=-6;
                                p_uzel->text="ERROR: Database open error";
                            }

                            db.close();
                        } // end of the block where the db object lives, it will be destroyed here

                        QSqlDatabase::removeDatabase(connectionName);

                    //insert to DB end




                }
                else
                {
                    p_uzel->text="No data in PLC archive";
                    ret=-4;
                }

            }
        }
        else
        {
            ret=-2;
            p_uzel->text="Error write query data to modbus";
        }



    }

        modbus_close(mb);
        modbus_free(mb);

return ret;
}
//==========================================================================================
int RecoveryHourGaz(uzel *p_uzel, uint year_,uint month_, uint day_,uint hour_)
{

    int ret=0;

    modbus_t *mb;
    uint16_t tab_reg[100];

    uint16_t flag;
    uint16_t line;

    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    float plotn_nom;
    float N_N2;
    float N_CO2;
    float D_it_20;
    float d_su_20;
    float a0_su;
    float a1_su;
    float a2_su;
    float Ra;
    float Rn;
    float Interval;


    //средних, делить на cycle_count
    float d_rTemp_avg;
    float d_rTisk_avg;
    float d_rPerepad_avg;
    float d_rVolFlowSU_avg;


    //накопленные
    float d_rObjemGazuSU;
    float d_rMassaGazu;  //d_rObjemGazonasZnevodnNafRU_MM_SOU

    float rep_rezerv1;
    float rep_rezerv2;


    uint16_t alarmsCode;
    uint16_t alarmsTimeSec;
    uint16_t WhichKoeffSaved;


    mb = modbus_new_tcp(p_uzel->IP_addr.toStdString().c_str(), 502);

    if (modbus_connect(mb)!=0 )
    {
        //QMessageBox::information(this,"Test","Зв'язок з об'єктом відсутній!!!",QMessageBox::Ok);
        ret=-1;
        p_uzel->text="Error connect to modbus interface";
        //emit textchange(i, "No connection with source");
    }
    else //connect OK
    {
        modbus_set_slave(mb, 1);

        tab_reg[FLAG]=1;
        tab_reg[LINE]=1;
        tab_reg[YEAR]=year_;
        tab_reg[MONTH]=month_;
        tab_reg[DAY]=day_;
        tab_reg[HOUR]=hour_;

        if (modbus_write_registers(mb, 499, 7, tab_reg)==7)  //query data
        {
            Sleep(3000);

            int res=modbus_read_registers(mb, 499, 51, tab_reg);

            if (res!=51)
            {
                ret=-3;
                p_uzel->text="Error read from modbus";

            }
            else //read OK
            {

                flag=tab_reg[0];
                line=tab_reg[1];
                year=tab_reg[3];
                month=tab_reg[4];
                day=tab_reg[5];
                hour=tab_reg[6];

                if (flag==0 && line==1 && year==year_ && month==month_ && day==day_ && hour==hour_)
                {
                    plotn_nom=modbus_get_float(&tab_reg[7]);
                     N_N2=modbus_get_float(&tab_reg[9]);
                     N_CO2=modbus_get_float(&tab_reg[11]);
                     D_it_20=modbus_get_float(&tab_reg[13]);
                     d_su_20=modbus_get_float(&tab_reg[15]);
                     a0_su=modbus_get_float(&tab_reg[17]);
                     a1_su=modbus_get_float(&tab_reg[19]);
                     a2_su=modbus_get_float(&tab_reg[21]);


                     Ra=modbus_get_float(&tab_reg[23]);
                     Rn=modbus_get_float(&tab_reg[25]);
                     Interval=modbus_get_float(&tab_reg[27]);
                     //средних, делить на cycle_count
                     d_rTemp_avg=modbus_get_float(&tab_reg[29]);
                     d_rTisk_avg=modbus_get_float(&tab_reg[31]);
                     d_rPerepad_avg=modbus_get_float(&tab_reg[33]);
                     d_rVolFlowSU_avg=modbus_get_float(&tab_reg[35]);
                     //накопленные
                     d_rObjemGazuSU=modbus_get_float(&tab_reg[37]);
                     d_rMassaGazu=modbus_get_float(&tab_reg[39]);
                     //rep_rezerv1=modbus_get_float(&tab_reg[41]);
                     //rep_rezerv2=modbus_get_float(&tab_reg[43]);

                    alarmsCode=tab_reg[45];
                    alarmsTimeSec=tab_reg[46];
                    WhichKoeffSaved=tab_reg[47];


                    QString connectionName=GetNextName();

                        {  // start of the block where the db object lives

                            QSqlDatabase db = QSqlDatabase::addDatabase("QODBC",connectionName);
                                  db.setDatabaseName(p_uzel->OdbcName);
                                  db.setUserName("sysdba");//user);
                                  db.setPassword("784523");//pass);
                            if (db.open())
                            {

                                QSqlQuery sqlQuery(db);
                                QString query;

                                    query.sprintf(QString("INSERT INTO " + p_uzel->TableName + "(" +
                                                          "DT, TEMP, TISK, PEREPAD, VOLFLOWSU, " +
                                                          "PLOTN_NOM, N_N2, N_CO2, D_IT_20, D_SU_20, A0_SU, A1_SU, A2_SU, " +
                                                          "RA, RN, INTERVAL, OBJEMGAZUSU, MASSAGAZU, "+
                                                          "ALARMSCODE , ALARMSTIMESEC, WHICHKOEFFSAVED) "
                                                          "VALUES ("+
                                                          "'%i.%i.%i %i:00:00', %f, %f, %f, %f, "+
                                                          "%f, %f, %f, %f, %f, %f, %f, %f, " +
                                                          "%f, %f, %f, %f, %f, " +
                                                          "%i, %i, %i)").toStdString().c_str(),
                                                          day,month,year,hour,d_rTemp_avg,d_rTisk_avg,d_rPerepad_avg,d_rVolFlowSU_avg,
                                                          plotn_nom,N_N2,N_CO2,D_it_20,d_su_20,a0_su,a1_su,a2_su,
                                                          Ra,Rn,Interval,d_rObjemGazuSU,d_rMassaGazu,
                                                          alarmsCode, alarmsTimeSec, WhichKoeffSaved
                                                          );

                                                  //  emit insert(query);

                                    sqlQuery.exec(query);

                                    if (sqlQuery.lastError().isValid())
                                    {
                                        //QMessageBox::critical(NULL, QObject::tr("Database Error"), sqlQuery.lastError().text());
                                        //emit textchange(i,"ERROR: Database error on INSERT");
                                        ret=-5;
                                        p_uzel->text="ERROR: Database error on INSERT";
                                    }
                                    else
                                    {
                                        //QMessageBox::information(NULL, tr("Информация"), "Запись удалена успешно.");
                                        //emit textchange(i,"INSERT OK");
                                        ret=1;
                                        p_uzel->text="Data recover succesfully";
                                    }




                            }
                            else
                            {
                                //QMessageBox::critical(NULL, QObject::tr("Database Error"), db.lastError().text());
                                //emit insert(db.lastError().databaseText());
                                //emit insert(db.lastError().driverText());
                                //emit textchange(i,"ERROR: Database not open");
                                ret=-6;
                                p_uzel->text="ERROR: Database open error";
                            }

                            db.close();
                        } // end of the block where the db object lives, it will be destroyed here

                        QSqlDatabase::removeDatabase(connectionName);

                    //insert to DB end




                }
                else
                {
                    p_uzel->text="No data in PLC archive";
                    ret=-4;
                }

            }
        }
        else
        {
            ret=-2;
            p_uzel->text="Error write query data to modbus";
        }



    }

        modbus_close(mb);
        modbus_free(mb);

return ret;

}
//==========================================================================================
int RecoveryHour(uzel *p_uzel, uint year_,uint month_, uint day_,uint hour_)
{
    int ret=0;
    if (p_uzel->type=="vosn") {ret= RecoveryHourVosn(p_uzel,year_,month_, day_,hour_);}
    if (p_uzel->type=="1uvr2nord") {ret= RecoveryHour1Uvr2Nord(p_uzel, year_,month_, day_,hour_);}
    if (p_uzel->type=="1uvr2nord1mm") {ret= RecoveryHour1Uvr2Nord1MM(p_uzel, year_,month_, day_,hour_);}
    if (p_uzel->type=="4uvr") {ret= RecoveryHour4Uvr(p_uzel, year_,month_, day_,hour_);}
    if (p_uzel->type=="gaz") {ret= RecoveryHourGaz(p_uzel, year_,month_, day_,hour_);}
return ret;
}

//==========================================================================================
void MainWindow::PushButtonCheckHour()
{
/*
    int number_in_uzel_array=ui->listWidget->currentRow();



    int year=ui->calendarWidget->selectedDate().year();
    int month=ui->calendarWidget->selectedDate().month();
    int day=ui->calendarWidget->selectedDate().day();
    int hour=ui->timeEdit->time().hour();

    uzels[number_in_uzel_array].dt.sprintf("%.2u.%.2u.%.4u %.2u:00:00",day,month,year,hour);

    ui->listWidget_history->addItem("          <<<<<<<       Check Hour : " + uzels[number_in_uzel_array].ObjectName+" : " +uzels[number_in_uzel_array].dt +"       >>>>>>>");
    ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::darkGreen);


    if (CheckHour(&uzels[number_in_uzel_array])==1)
    {
        ui->listWidget_history->addItem(uzels[number_in_uzel_array].ObjectName+" : " + uzels[number_in_uzel_array].dt+ " === " +uzels[number_in_uzel_array].text);
        ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::black);
    }
    else
    {
        ui->listWidget_history->addItem(uzels[number_in_uzel_array].ObjectName+" : " + uzels[number_in_uzel_array].dt+ " === " +uzels[number_in_uzel_array].text);
        ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::red);
    }

    ui->listWidget_history->scrollToBottom();
*/
}
//============================================================================================
void MainWindow::PushButtonCheckDay()
{

    int number_in_uzel_array=ui->listWidget->currentRow();



    int year=ui->calendarWidget->selectedDate().year();
    int month=ui->calendarWidget->selectedDate().month();
    int day=ui->calendarWidget->selectedDate().day();

    QString tempdt;
    tempdt.sprintf("%.2u.%.2u.%.4u",day,month,year);

    ui->listWidget_history->addItem("          <<<<<<<       Check Day : " + uzels[number_in_uzel_array].ObjectName+" : " +tempdt +"       >>>>>>>");
    ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::darkGreen);


    for(int hour=0;hour<24;++hour)
    {
        uzels[number_in_uzel_array].dt.sprintf("%.2u.%.2u.%.4u %.2u:00:00",day,month,year,hour);

        if (CheckHour(&uzels[number_in_uzel_array])==1)
        {
            ui->listWidget_history->addItem(uzels[number_in_uzel_array].ObjectName+" : " + uzels[number_in_uzel_array].dt+ " === " +uzels[number_in_uzel_array].text);
            ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::black);
        }
        else
        {
            ui->listWidget_history->addItem(uzels[number_in_uzel_array].ObjectName+" : " + uzels[number_in_uzel_array].dt+ " === " +uzels[number_in_uzel_array].text);
            ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::red);
        }

        ui->listWidget_history->scrollToBottom();
        QApplication::processEvents();
    }

}
//============================================================================================
void MainWindow::PushButtonRecoveryHour()
{
/*
    int number_in_uzel_array=ui->listWidget->currentRow();



    int year=ui->calendarWidget->selectedDate().year();
    int month=ui->calendarWidget->selectedDate().month();
    int day=ui->calendarWidget->selectedDate().day();
    int hour=ui->timeEdit->time().hour();

    uzels[number_in_uzel_array].dt.sprintf("%.2u.%.2u.%.4u %.2u:00:00",day,month,year,hour);

    ui->listWidget_history->addItem("          <<<<<<<       Recovery Hour : " + uzels[number_in_uzel_array].ObjectName+" : " +uzels[number_in_uzel_array].dt +"       >>>>>>>");
    ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::darkGreen);

    if (CheckHour(&uzels[number_in_uzel_array])>0)  //no data in DB
    {
        uzels[number_in_uzel_array].text="Data is present in DB";
        ui->listWidget_history->addItem(uzels[number_in_uzel_array].ObjectName+" : " + uzels[number_in_uzel_array].dt+ " === " +uzels[number_in_uzel_array].text);
        ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::red);
    }
    else
    {
        if (RecoveryHour(&uzels[number_in_uzel_array],year,month,day,hour)==1)
        {
            ui->listWidget_history->addItem(uzels[number_in_uzel_array].ObjectName+" : " + uzels[number_in_uzel_array].dt+ " === " +uzels[number_in_uzel_array].text);
            ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::black);
        }
        else
        {
            ui->listWidget_history->addItem(uzels[number_in_uzel_array].ObjectName+" : " + uzels[number_in_uzel_array].dt+ " === " +uzels[number_in_uzel_array].text);
            ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::red);
        }

    }
    ui->listWidget_history->scrollToBottom();
*/
}
//============================================================================================
void MainWindow::PushButtonRecoveryDay()
{
    int number_in_uzel_array=ui->listWidget->currentRow();



    int year=ui->calendarWidget->selectedDate().year();
    int month=ui->calendarWidget->selectedDate().month();
    int day=ui->calendarWidget->selectedDate().day();

    QString tempdt;
    tempdt.sprintf("%.2u.%.2u.%.4u",day,month,year);

    ui->listWidget_history->addItem("          <<<<<<<       Recovery Day : " + uzels[number_in_uzel_array].ObjectName+" : " +tempdt +"       >>>>>>>");
    ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::darkGreen);


    for(int hour=0;hour<24;++hour)
    {
        uzels[number_in_uzel_array].dt.sprintf("%.2u.%.2u.%.4u %.2u:00:00",day,month,year,hour);

        if (CheckHour(&uzels[number_in_uzel_array])>0)  //no data in DB
        {
            uzels[number_in_uzel_array].text="Data is present in DB";
            ui->listWidget_history->addItem(uzels[number_in_uzel_array].ObjectName+" : " + uzels[number_in_uzel_array].dt+ " === " +uzels[number_in_uzel_array].text);
            ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::red);
        }
        else
        {

            if (RecoveryHour(&uzels[number_in_uzel_array],year,month,day,hour)==1)
            {
                ui->listWidget_history->addItem(uzels[number_in_uzel_array].ObjectName+" : " + uzels[number_in_uzel_array].dt+ " === " +uzels[number_in_uzel_array].text);
                ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::black);
            }
            else
            {
                ui->listWidget_history->addItem(uzels[number_in_uzel_array].ObjectName+" : " + uzels[number_in_uzel_array].dt+ " === " +uzels[number_in_uzel_array].text);
                ui->listWidget_history->item(ui->listWidget_history->count()-1)->setTextColor(Qt::red);
            }
        }

    ui->listWidget_history->scrollToBottom();
    QApplication::processEvents();
    }
}
//============================================================================================
