#include <stdio.h>
//#include <wiringPi.h>
#include "pinsworker.h"

PinsWorker &PinsWorker::getInstance()
{
  static PinsWorker worker;
  return worker;
}

PinsWorker::PinsWorker(QObject *parent) : QObject(parent)
{

}
