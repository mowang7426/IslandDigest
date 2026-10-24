#include <assert.h>
#include <stdio.h>
#include "../IDCollapseDiagnostics.h"
int main(void) {
    IDCollapseDiagnosticState s = {0};
    assert(s.event == 0 && s.total[0] == 0 && s.total[1] == 0 && s.total[2] == 0);
    for (int phase = 0; phase < 6; phase++) {
        uint64_t n = IDCollapseDiagnosticEnter(&s, 0, phase, 1, 1, "TestRecognizer");
        IDCollapseDiagnosticExit(&s, 0, n, phase);
        assert(s.states[0][phase] == 1 && s.postStates[0][phase] == 1);
    }
    for (unsigned n = 0; n < 100000; n++) {
        uint64_t event = IDCollapseDiagnosticEnter(&s, 0, 2, 1, 1, "TestRecognizer");
        IDCollapseDiagnosticExit(&s, 0, event, 2);
    }
    assert(s.total[0] == 100006 && s.states[0][2] == 100001);
    uint64_t outer = IDCollapseDiagnosticEnter(&s, 1, 0, 1, 1, "TestRecognizer");
    uint64_t inner = IDCollapseDiagnosticEnter(&s, 1, 1, 1, 1, "TestRecognizer");
    IDCollapseDiagnosticExit(&s, 1, inner, 2);
    IDCollapseDiagnosticExit(&s, 1, outer, 5);
    assert(s.post[1] == 2 && s.postEvent[1] == inner);
    IDCollapseDiagnosticEnter(&s, 2, -1, 0, 0, "NSObject");
    assert(s.offMain[2] == 1 && s.states[2][6] == 1 && s.recognizer[2] == 0);
    assert(s.total[0] == 100006 && s.total[1] == 2 && s.total[2] == 1);
    puts("PASS production raw counters: zero, all phases, 100k Changed, nested orig ordering, off-main and separate probes");
}
