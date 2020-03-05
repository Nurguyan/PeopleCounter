#include "person.h"
#include "opencv2/opencv.hpp"
#include <QString>

using namespace cv;

Person::Person(int ID, Point point, QString direction)
{
    this->ID = ID;
    coordinate = point;
    direct = direction;
}

Person::Person(Point point, QString direction)
{
    this->ID = ID;
    coordinate = point;
    direct = direction;
}

void Person::setID(int id){
    this->ID = id;
}

void Person::operator=(const Person &other){
    ID = other.ID;
    coordinate = other.coordinate;
}

bool Person::operator==(const Person &other) const{
    if (this->ID == other.ID)
        return true;
    else {
        return false;
    }
}
