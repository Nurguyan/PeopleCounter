#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "person.h"

#include <QMainWindow>
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>

#include <QString>
#include <QList>
#include <QSet>

#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>


using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->le_filename->setText("");
    ui->graphicsView->setScene(new QGraphicsScene(this));
    ui->graphicsView->scene()->addItem(&pixmap);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_File_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open video"), "",tr("MP4 video (*.mp4);;All Files (*)"));
    ui->le_filename->setText(fileName);
    ui->bt_playstop->animateClick();
}

void MainWindow::on_bt_playstop_clicked()
{
    QString fileName = ui->le_filename->text();

    if (!video.isOpened()){
        int IDiter = 0;
        if (fileName.isEmpty()){
            QMessageBox::critical(this, "Error", "Please, choose video file");
            return;
        }

        if (!video.open(fileName.toStdString(), CAP_FFMPEG)){
                QMessageBox::critical(this,"Video Error","Make sure you entered a correct and supported video file path");
                return;
        }

        int in = 0, out = 0;
        Mat frame, gray, frameDelta, thresh, firstFrame;
        std::vector<std::vector<Point>> cnts;
        QSet<Person> people;
        QList<Rect2d> prev_boxes;
        const int minArea = 700;
        int frame_width = video.get(CAP_PROP_FRAME_WIDTH);
        int frame_height = video.get(CAP_PROP_FRAME_HEIGHT);
        video >> frame;
        if (frame.empty()) {
            QMessageBox::critical(this, "Error", "Could not read video file");
            return;
        }

        ui->bt_playstop->setText("Stop");

        //convert to grayscale and set the first frame
        cvtColor(frame, firstFrame, COLOR_BGR2GRAY);
        GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);

        while(video.isOpened()) {
            video >> frame;

            if (frame.empty()) {
                video.release();
                ui->bt_playstop->setText("Play");
                break;
            }

            //convert to grayscale
            cvtColor(frame, gray, COLOR_BGR2GRAY);
            GaussianBlur(gray, gray, Size(21, 21), 0);

            //compute difference between first frame and current frame
            absdiff(firstFrame, gray, frameDelta);

            threshold(frameDelta, thresh, 65, 255, THRESH_BINARY);
            dilate(thresh, thresh, Mat(), Point(-1,-1), 2);
            findContours(thresh, cnts, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

            QList<Rect2d> boxes;

            for(int i = 0; i < cnts.size(); i++) {
                if(contourArea(cnts[i]) < minArea) {
                    continue;
                }
                Rect2d bound_box = boundingRect(cnts[i]);
                boxes.append(bound_box);
                //draw bound box
                //rectangle(frame, bound_box, Scalar(0,0,255), 2, 0);
            }


            if (!boxes.empty() && prev_boxes.empty()){
                for(int i = 0; i < boxes.size(); i++){
                    IDiter++;
                    Point box_centroid = (boxes[i].br() + boxes[i].tl())*0.5;
                    QString direction;
                    if (box_centroid.x <= video.get(CAP_PROP_FRAME_WIDTH) / 2){
                        direction = "IN";
                        in++;
                    }
                    else{
                        direction = "OUT";
                        out++;
                    }

                    Person per(IDiter, box_centroid, direction);
                    people.insert(per);
                    qDebug().nospace().noquote() << "Person added: ID("<<per.ID<<") direction(" + per.direct + ")";
                }
            }
            else
            if (!boxes.empty() && !prev_boxes.empty()){
                QList<Person> newpeople = getNewPersons(boxes, prev_boxes);
                for(Person p: newpeople){
                    IDiter++;
                    p.setID(IDiter);
                    people.insert(p);
                    if (p.direct == "IN") in++;
                    else
                    if (p.direct == "OUT") out++;
                    qDebug().nospace().noquote() << "Person added: ID("<<p.ID<<") direction(" + p.direct + ")";
                }
            }

            prev_boxes = boxes;

            //display
            putText(frame, "OUT: " + QString::number(out).toStdString(),
                    Point(frame_width/6, frame_height/8*7), FONT_HERSHEY_DUPLEX, 1, Scalar(200,60,60), 2);
            putText(frame, "IN: " + QString::number(in).toStdString(),
                    Point(frame_width/6*4, frame_height/8*7), FONT_HERSHEY_DUPLEX, 1, Scalar(200,60,60), 2);

            showFrame(&frame, &pixmap, ui->graphicsView);
            //imshow("video", frame);
            //waitKey(1000/video.get(CAP_PROP_FPS));
            //waitKey(1000/80);
            ui->lb_in_num->setText(QString::number(in));
            ui->lb_out_num->setText(QString::number(out));
            qApp->processEvents();
        }
        qDebug() << "Total number of people:" << people.size();
        qDebug() << "OUT:" << out;
        qDebug() << "IN:" << in;

    }
    else {
        video.release();
        ui->bt_playstop->setText("Play");
    }
}

QList<Person> MainWindow::getNewPersons(QList<Rect2d> boxes, QList<Rect2d> prev_boxes){
    QList<Person> newpeople;
    const int dx_thresh = 200;

    for(int i = 0; i < boxes.size(); i++){
        double min_dist = INFINITY;
        int mindist_j = 0;
        Point box_centroid = (boxes[i].br() + boxes[i].tl())*0.5;

        //find closest boxes assuming they are the same person
        for(int j = 0; j < prev_boxes.size(); j++){
            Point prevbox_centroid = (prev_boxes[j].br() + prev_boxes[j].tl())*0.5;
            double dist = abs(norm(box_centroid - prevbox_centroid));
            if (dist < min_dist) {
                min_dist = dist;
                mindist_j = j;
            }
        }

        //if distance above threshold then it is a new person
        if (min_dist > dx_thresh){
            Point XY = (prev_boxes[mindist_j].br() + prev_boxes[mindist_j].tl())*0.5;
            QString direction;
            if (XY.x <= video.get(CAP_PROP_FRAME_WIDTH) / 2){
                direction = "IN";
            }
            else{
                direction = "OUT";
            }
            Person per(XY, direction);
            newpeople.append(per);
            prev_boxes.removeAt(mindist_j);
            if (prev_boxes.empty()) return newpeople;
        }
    }
    return newpeople;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(video.isOpened()) video.release();
    ui->bt_playstop->setText("Play");
    return;
}

void MainWindow::showFrame(Mat *frame, QGraphicsPixmapItem *pixmap, QGraphicsView *graphicsView){
    if (frame->empty()) return;
    QImage qimg(frame->data, frame->cols, frame->rows, frame->step,QImage::Format_RGB888);
    pixmap->setPixmap(QPixmap::fromImage(qimg.rgbSwapped()));
    graphicsView->fitInView(pixmap, Qt::KeepAspectRatio);
}

