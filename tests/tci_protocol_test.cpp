#include "../esp32-backend/TciProtocol.h"

#include <assert.h>
#include <iostream>
#include <string.h>
#include <string>

static void feed(TciProtocol &protocol, const char *text) {
  protocol.feed(reinterpret_cast<const uint8_t *>(text), strlen(text));
}

int main() {
  TciProtocol protocol;
  assert(!protocol.state().runningKnown);
  feed(protocol, "protocol:ExpertSDR3,2.1;device:SunSDR2DX;ready;");
  assert(!protocol.state().runningKnown);  // READY does not mean the radio is on.
  feed(protocol, "START;VFO:0,0,7100123;MODULATION:0,LSB;");
  assert(protocol.state().runningKnown && protocol.state().running);
  assert(protocol.state().frequencyValid && protocol.state().frequencyHz == 7100123);
  assert(protocol.state().modulationValid && strcmp(protocol.state().modulation, "LSB") == 0);

  feed(protocol, "vfo:1,0,14000000;vfo:0,1,21000000;modulation:1,USB;trx:0,false;");
  assert(protocol.state().frequencyHz == 7100123);
  assert(strcmp(protocol.state().modulation, "LSB") == 0);
  assert(protocol.state().running);  // TRX is PTT, not power.

  feed(protocol, "\n vFo : 0, 0, 1407");
  assert(protocol.state().frequencyHz == 7100123);
  feed(protocol, "4000 ; modulation:0,digu;\r\n");
  assert(protocol.state().frequencyHz == 14074000);
  assert(strcmp(protocol.state().modulation, "DIGU") == 0);

  feed(protocol, "vfo:0,0,-1;vfo:0,0,0;vfo:0,0,4294967296;vfo:0,0,123abc;");
  feed(protocol, "vfo:0,0,7000000,extra;modulation:0,;modulation:0,USB\";start:wrong;");
  assert(protocol.state().frequencyHz == 14074000);
  assert(strcmp(protocol.state().modulation, "DIGU") == 0);

  const std::string oversized(512, 'x');
  feed(protocol, oversized.c_str());
  feed(protocol, "vfo:0,0,10000000;vfo:0,0,28000000;");
  assert(protocol.state().frequencyHz == 28000000);
  const uint8_t nulCommand[] = {'s', 't', 'o', 'p', 0, ';'};
  protocol.feed(nulCommand, sizeof(nulCommand));
  assert(protocol.state().running);

  feed(protocol, "stop;");
  assert(protocol.state().runningKnown && !protocol.state().running);
  assert(!protocol.state().frequencyValid && !protocol.state().modulationValid);
  feed(protocol, "start;vfo:0,0,3500000;modulation:0,cw;");
  assert(protocol.state().running && protocol.state().frequencyHz == 3500000);
  protocol.reset();  // Disconnect/reconnect must discard partial commands and old state.
  assert(!protocol.state().runningKnown && !protocol.state().frequencyValid && !protocol.state().modulationValid);
  feed(protocol, "vfo:0,0,71");
  protocol.reset();
  feed(protocol, "00000;");
  assert(!protocol.state().frequencyValid);

  std::cout << "TCI protocol tests passed\n";
}
