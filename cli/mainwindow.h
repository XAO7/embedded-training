#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_connect_clicked();

    void recvData();
    void myconnected();
    void mydisconnected();
    void myerror(QAbstractSocket::SocketError);

    void on_pushButton_discon_clicked();

    void on_pushButton_up_clicked();

    void on_pushButton_down_clicked();

private:
    Ui::MainWindow *ui;
    QTcpSocket sock;
    bool sizeRead = false;
    int size;
    char buf[1024000];
};
#endif // MAINWINDOW_H
