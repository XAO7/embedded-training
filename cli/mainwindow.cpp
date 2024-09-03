#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lineEdit_ip->setText("192.168.6.88");
    ui->lineEdit_port->setText("54321");

    ui->pushButton_up->setEnabled(false);
    ui->pushButton_down->setEnabled(false);

    connect(&sock, SIGNAL(readyRead()), this, SLOT(recvData()));
    connect(&sock, SIGNAL(connected()), this, SLOT(myconnected()));
    connect(&sock, SIGNAL(disconnected()), this, SLOT(mydisconnected()));
    connect(&sock,QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &MainWindow::myerror);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::myerror(QAbstractSocket::SocketError socketError)
{
    QString errorString = "Unknown Error";
    switch (socketError) {
    case QAbstractSocket::ConnectionRefusedError:
        errorString = "The connection was refused by the peer.";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorString = "The host address was not found.";
        break;
    // ... 其他错误类型处理 ...
    default:
        errorString = sock.errorString();
        break;
    }
    qDebug() << "Socket error occurred:" << errorString;
}

void MainWindow::myconnected()
{
    qDebug()<<"connect ok";
    ui->textEdit->append("connect ok");
}
void MainWindow::mydisconnected()
{
    qDebug() << "disconnect ok";
    ui->textEdit->append("disconnect ok");
}

void MainWindow::recvData()
{
    if (sizeRead == false)
    {
        sock.read((char *)&size, 4);
        sizeRead = true;
        qDebug() << "size:" << size;
    }
    else
    {
        qDebug() << "sock avail:" << sock.bytesAvailable();
        if (sock.bytesAvailable() >= size)
        {
            qDebug() << "sock read:" << sock.read(buf, size);
            QPixmap pix;
            pix.loadFromData((const uchar *)buf, size, "JPEG");
            ui->label_image->setPixmap(pix);
            ui->label_2->show();

            sizeRead = false;
        }
    }
}

void MainWindow::on_pushButton_connect_clicked()
{
    sock.connectToHost(ui->lineEdit_ip->text(), ui->lineEdit_port->text().toInt());
    sock.waitForConnected();
}

void MainWindow::on_pushButton_discon_clicked()
{
    sock.disconnectFromHost();
    sizeRead = false;
}

void MainWindow::on_pushButton_up_clicked()
{

}

void MainWindow::on_pushButton_down_clicked()
{

}
