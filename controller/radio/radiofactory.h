#ifndef RADIOFACTORY_H
#define RADIOFACTORY_H

class IRadio;

class RadioFactory
{
public:
  static IRadio* getRadio();
};

#endif // RADIOFACTORY_H
