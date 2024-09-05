#include "protocol.h"

void protocol_request_to_le(const BEProtocolRequest *be,
                            LEProtocolRequest *le) {}
void protocol_response_to_le(const BEProtocolResponse *be,
                             LEProtocolResponse *le) {}
void protocol_request_to_be(const LEProtocolRequest *le,
                            BEProtocolRequest *be) {}
void protocol_response_to_be(const LEProtocolResponse *le,
                             BEProtocolResponse *be) {}
