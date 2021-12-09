/*
 * Copyright (c) 2020-2021. demo Software Ltd. All rights reserved.
 *
 * Author:     xxxxxx <xxxxxx@163.com>
 *
 * Maintainer: xxxxxx <xxxxxx@163.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>
#include <string>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QDateTime>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>

using namespace std;

bool dirExists(const QString &path)
{
    QFileInfo fs(path);
    return fs.exists() && fs.isDir() ? true : false;
}

void Qsqltest()
{
    const QString dbPath = "/deepin/xxxxxx/layers/";
    // 添加数据库驱动，并指定连接名称package_connection
    QSqlDatabase dbConn;
    if (QSqlDatabase::contains("package_connection")) {
        dbConn = QSqlDatabase::database("package_connection");
    } else {
        dbConn = QSqlDatabase::addDatabase("QSQLITE", "package_connection");
        dbConn.setDatabaseName(dbPath + "AppInstalledInfo.db");
    }
    if (!dbConn.open()) {
        qCritical() << "open DB failed";
        return;
    }

    qInfo() << "db status:" << dbConn.isOpen();

    QString createSql = "CREATE TABLE IF NOT EXISTS student(\
         ID INTEGER PRIMARY KEY AUTOINCREMENT,\
         name VARCHAR(32) NOT NULL,\
         sex CHAR(10),\
         score INTEGER\
         )";
    QSqlQuery sql_query(createSql, dbConn);
    // sql_query.prepare(dbConn, createSql);
    if (!sql_query.exec()) {
        qInfo() << "Error: Fail to create table." << sql_query.lastError();
    } else {
        qInfo() << "Table created success";
    }
    // 增
    QString insertSql = "INSERT INTO student(name,sex,score) VALUES('zhangsan', '男','78')";
    sql_query.prepare(insertSql);
    if (!sql_query.exec()) {
        qInfo() << "Error: Fail to exec sql:" << insertSql << ", error:" << sql_query.lastError();
        return;
    }

    insertSql = "INSERT INTO student(name,sex,score) VALUES('lili', '女','80')";
    sql_query.prepare(insertSql);
    if (!sql_query.exec()) {
        qInfo() << "Error: Fail to exec sql:" << insertSql << ", error:" << sql_query.lastError();
        return;
    }

    insertSql = "INSERT INTO student(name,sex,score) VALUES('wangwu', '男','85')";
    sql_query.prepare(insertSql);
    if (!sql_query.exec()) {
        qInfo() << "Error: Fail to exec sql:" << insertSql << ", error:" << sql_query.lastError();
        return;
    }
    // 改
    QString updateSql = "update student set score=81,sex='女' where name='wangwu'";
    sql_query.prepare(updateSql);
    if (!sql_query.exec()) {
        qInfo() << "Error: Fail to exec sql:" << updateSql << ", error:" << sql_query.lastError();
        return;
    }
    // 删
    QString delSql = "delete from student where name = 'lili'";
    sql_query.prepare(delSql);
    if (!sql_query.exec()) {
        qInfo() << "Error: Fail to exec sql:" << delSql << ", error:" << sql_query.lastError();
        return;
    }
    // 查
    QString select_sql = "select * from student";
    if (!sql_query.exec(select_sql)) {
        qInfo() << sql_query.lastError();
        return;
    } else {
        while (sql_query.next()) {
            int id = sql_query.value(0).toInt();
            QString name = sql_query.value(1).toString();
            qInfo() << QString("id:%1 name:%2").arg(id).arg(name);
        }
    }
    //关闭数据库
    dbConn.close();
}

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;
    mutex.lock();
    QString text;
    switch (type) {
    case QtDebugMsg:
        text = QString("[Debug]");
        break;
    case QtInfoMsg:
        text = QString("[Info]");
        break;
    case QtWarningMsg:
        text = QString("[Warning]");
        break;
    case QtCriticalMsg:
        text = QString("[Critical]");
        break;
    case QtFatalMsg:
        text = QString("[Fatal]");
    }

    QString context_info =
        QString("[File:%1 Line:%2 Func:%3]").arg(QString(context.file)).arg(context.line).arg(context.function);

    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    QString current_date = QString("%1").arg(current_date_time);

    QString message = QString("%1 %2 %3 %4").arg(current_date).arg(text).arg(context_info).arg(msg);

    QFile file("/tmp/ll-debug/ostreehelp_log.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream textStream(&file);
    textStream << message << "\n";
    file.flush();
    file.close();
    mutex.unlock();
}

// curl --location --request POST 'http://10.20.54.2:8888/xxxxxx/app/fuzzySearchApp' --header 'Content-Type:
// application/json' --data '{"AppId":"org.xxxxxx.calculator"}'
// 建议使用qtcreator调试
void testHttpPost(const QString &pkgName, const QString &pkgVer, const QString &pkgArch)
{
    QNetworkAccessManager mgr;
    const QUrl url(QStringLiteral("http://10.20.54.2:8888/xxxxxx/app/fuzzySearchApp"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject obj;
    obj["appId"] = pkgName;
    obj["version"] = pkgVer;
    obj["arch"] = pkgArch;
    QJsonDocument doc(obj);
    // 如果转换成QString QT QString 中如果有""则会自动反斜杠\转义 此处转化为 QByteArray
    QByteArray data = doc.toJson();
    QNetworkReply *reply = mgr.post(request, data);
    QString responseData;
    QEventLoop eventLoop;
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, [&]() {
        if (reply->error() == QNetworkReply::NoError) {
            responseData = QString::fromUtf8(reply->readAll());
            qInfo().noquote() << "retMsg:" << responseData;
        } else {
            QString err = reply->errorString();
            qCritical() << "err msg:" << err;
        }
        reply->deleteLater();
        eventLoop.quit();
    });
    // 5s 超时
    QTimer::singleShot(5000, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
    qInfo().noquote() << "testHttpPost done";
}

int main(int argc, char **argv)
{
    // 需要初始化这个 不然事件testHttpPost循环不起作用
    QCoreApplication app(argc, argv);

    if (dirExists("/tmp/ll-debug")) {
        qInstallMessageHandler(outputMessage);
    } else {
        qSetMessagePattern(
            "%{time yyyy-MM-dd hh:mm:ss.zzz} [%{type}] [File:%{file} Line:%{line} Function:%{function}] %{message}");
    }
    // Qsqltest();
    testHttpPost("runtime", "", "");
}
