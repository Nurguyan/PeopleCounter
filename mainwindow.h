#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QList>

#include "person.h"

#include "opencv2/opencv.hpp"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);
    void showFrame(cv::Mat *frame, QGraphicsPixmapItem *pixmap, QGraphicsView *graphicsView);
    QList<Person> getNewPersons(QList<cv::Rect2d> boxes, QList<cv::Rect2d> prev_boxes);

private slots:
    void on_actionOpen_File_triggered();
    void on_bt_playstop_clicked();

private:
    Ui::MainWindow *ui;
    QGraphicsPixmapItem pixmap;
    cv::VideoCapture video;
};

#endif // MAINWINDOW_H
