#pragma once

namespace SerialManager {
	void handle();
	void printHelp();
}

// Backward-compatible entrypoint used by main loop.
void handleSerialCommands();
