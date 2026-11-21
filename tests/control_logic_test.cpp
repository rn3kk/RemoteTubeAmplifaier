#include "../esp32-backend/ControlLogic.h"

#include <assert.h>
#include <iostream>

int main() {
  Point points[] = {{7100, 100, 200, 1, true}, {7125, 300, 400, 2, false},
                    {7150, 500, 600, 3, true}, {14000, 700, 800, 4, true}};
  assert(frequencyCell(points, 4, 7087499) == -1);
  assert(frequencyCell(points, 4, 7087500) == 0);
  assert(frequencyCell(points, 4, 7112499) == 0);
  assert(frequencyCell(points, 4, 7112500) == 1);
  assert(frequencyCell(points, 4, 7137499) == 1);
  assert(frequencyCell(points, 4, 7137500) == 2);
  assert(frequencyCell(points, 4, 7162500) == 2);
  assert(frequencyCell(points, 4, 7162501) == -1);
  assert(frequencyCell(points, 4, 10000000) == -1);
  assert(frequencyCell(points, 4, UINT32_MAX) == -1);
  assert(frequencyCell(points, 0, 7100000) == -1);
  Point reverse[] = {points[2], points[1], points[0]};
  assert(frequencyCell(reverse, 3, 7112500) == 1);

  TciRadioState radio;
  assert(selectAutomaticPoint(points, 4, false, radio).block == AutoBlock::NoConnection);
  assert(selectAutomaticPoint(points, 4, true, radio).block == AutoBlock::RadioUnknown);
  radio.runningKnown = true;
  assert(selectAutomaticPoint(points, 4, true, radio).block == AutoBlock::RadioOff);
  radio.running = true;
  assert(selectAutomaticPoint(points, 4, true, radio).block == AutoBlock::NoFrequency);
  radio.frequencyValid = true;
  radio.frequencyHz = 10000000;
  assert(selectAutomaticPoint(points, 4, true, radio).block == AutoBlock::NoCell);
  radio.frequencyHz = 7112000;
  assert(selectAutomaticPoint(points, 4, true, radio).index == 0);
  assert(selectAutomaticPoint(points, 4, true, radio).block == AutoBlock::None);
  radio.frequencyHz = 7113000;
  AutoSelection selection = selectAutomaticPoint(points, 4, true, radio);
  assert(selection.index == 1 && selection.block == AutoBlock::NotConfigured);
  // Do not substitute the adjacent saved cell for an unconfigured cell.
  points[1].saved = true;
  points[1].sw = 0;
  assert(selectAutomaticPoint(points, 4, true, radio).block == AutoBlock::NotConfigured);
  points[1].sw = 6;
  assert(selectAutomaticPoint(points, 4, true, radio).block == AutoBlock::NotConfigured);
  points[1].sw = 2;
  points[1].cap = TUNE_POS_MAX + 1;
  assert(selectAutomaticPoint(points, 4, true, radio).block == AutoBlock::NotConfigured);
  points[1].cap = TUNE_POS_MAX;
  assert(selectAutomaticPoint(points, 4, true, radio).block == AutoBlock::None);
  assert(selectAutomaticPoint(points, 4, false, radio).block == AutoBlock::NoConnection);
  std::cout << "Control logic tests passed\n";
}
