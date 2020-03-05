#ifndef PERSON_H
#define PERSON_H

#include "opencv2/opencv.hpp"
#include <QString>

class Person
{
public:
    int ID;
    cv::Point coordinate;
    QString direct;

    Person();
    Person(int ID, cv::Point point, QString direction);
    Person(cv::Point point, QString direction);
    void setID(int id);
    void operator=(const Person &other);
    bool operator==(const Person &other) const;


};

inline uint qHash(const Person &key){
    return key.ID;
}

#endif // PERSON_H
