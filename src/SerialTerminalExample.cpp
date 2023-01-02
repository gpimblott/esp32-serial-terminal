#include <SerialTerminal.hpp>

maschinendeck::SerialTerminal* term;

void addInt(String opts) {
	maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
	Serial.print(operands.first());
	Serial.print(" + ");
	Serial.print(operands.second());
	Serial.print(" = ");
	Serial.print(operands.first().toInt() + operands.second().toInt());
	Serial.print('\n');
}

void doNothing(String opts) {
	Serial.println("Serial Terminal Example commands:");
	term->printCommands();
}

void setup() {
	term = new maschinendeck::SerialTerminal(38400, "example");
	term->add("add", "<first> <second>", &addInt, "adds two integers");
	term->add("two", "", &doNothing, "This does nothing");
	term->add("longCommand", "<Some parameters>", &doNothing, "This also does nothing");
}

void loop() {
	term->loop();
}