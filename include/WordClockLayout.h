#pragma once

#include <Arduino.h>

void wordClockLayoutInit();
const String& wordClockLayoutActiveId();
const String& wordClockLayoutActiveName();
const String& wordClockLayoutText();
String wordClockLayoutWordPositionsJson();

bool wordClockLayoutApplyAndStore(const String& layoutId,
                                  const String& layoutName,
                                  const String& layoutText,
                                  const String& positionsJson,
                                  String& error);
