#pragma once

#define ST_VERSION "1.2"
#define CRLF "\r\n"
#define MAX_COMMANDS 64
#define CMD_DESCRIPTION_POS 30

#define ST_PRINT(str) Serial.print(str)
#define ST_PRINTLINE(str) Serial.println(str)

#include <Arduino.h>

namespace maschinendeck {

  #if not defined ST_FLAG_NOBUILTIN && defined E2END
  #include "EEPROM.h"
  void printEEPROM(String opts) {
    Serial.print("offset \t");
    for (uint8_t h = 0; h < 16; h++) {
        Serial.print(h, HEX);
        Serial.print('\t');
    }
    Serial.print("\r\n");
    for (uint8_t i = 0; i < (E2END / 16); i++) {
      String line = "";
      Serial.print(i * 16, HEX);
      Serial.print('\t');
      for (uint8_t n = 0; n <= 15; n++) {
        size_t value = EEPROM.read(i * 16 + n);
        Serial.print(value, HEX);
        Serial.print('\t');
        line += static_cast<char>(value);
      }
      Serial.print(line);
      Serial.print("\r\n");
    }
  }
  #endif

  /**
   * Representation of a command to be executed 
   */
  struct Command {
    String command;
    String paramDescription;
    void(*callback)(String param);
    String description;

    Command(String command,  
            String paramDescription,
            void(*callback)(String param), 
            String description) : command(command), 
            paramDescription(paramDescription),
            callback(callback), 
            description(description) {}
  };

  template <typename T, typename U>
  class Pair {
    private:
      T first_;
      U second_;
    public:
      Pair(T first, U second) : first_(first), second_(second) {}
      T first() {
        return this->first_;
      }

      U second() {
        return this->second_;
      }
  };

  /**
   * Main class that implements the terminal
   */
  class SerialTerminal {
    private:
      Command* commands[256];
      uint8_t size_;
      bool firstRun;
      String message;
      String prompt;

    public:
      SerialTerminal(long baudrate = 0, String prompt = "st") : size_(0), firstRun(true), message(""), prompt(prompt) {
        #if not defined ST_FLAG_NOBUILTIN && defined E2END
        this->add("eeprom", &printEEPROM, "prints the contents of EEPROM");
        #endif

        if (baudrate > 0) {
            Serial.begin(baudrate);
        }
        
        #ifndef ST_FLAG_NOHELP
        ST_PRINT(F("SerialTerm v"));
        ST_PRINTLINE(ST_VERSION);
        ST_PRINTLINE(F("(C) 2022, MikO - Hpsaturn & G.Pimblott"));
        ST_PRINTLINE(F("\tCommands:"));
        #endif
      }

      /**
       * Add a new command to check for
       */
      void add(String command,  String paramDescription, void(*callback)(String param), String description = "") {
        if (this->size_ >= MAX_COMMANDS)
          return;

        this->commands[this->size_] = new Command(command, paramDescription, callback, description);
        this->size_++;
      }

      /**
       * Print the defined commands to the conole
       */
      void printCommands() {
        for (uint8_t i = 0; i < this->size_; i++) {
          String command = this->commands[i]->command;
          String params = this->commands[i]->paramDescription;
          int commandLen = command.length();
          int paramsLen = params.length();
          int numSpaces = CMD_DESCRIPTION_POS - commandLen - paramsLen;

          std::string spaces((numSpaces>0?numSpaces:1), ' ');

          ST_PRINT("\t" + this->commands[i]->command + " " + params);
          ST_PRINT( spaces.c_str() );
          ST_PRINTLINE(this->commands[i]->description);
        }
      }

      /**
       * Parse the message, find the asscociated command and calls its action
       */
      bool findCommandAndCallAction(String message) {
        Pair<String, String> command = SerialTerminal::ParseCommand(message);
        bool found = false;
        for (uint8_t i = 0; i < this->size_ && !found; i++) {
          if (this->commands[i]->command == command.first()) {
            this->commands[i]->callback(command.second());
            found = true;
          }
        }

        // If we didn't find the command then tell the user
        if (!found) {
            ST_PRINT("\n"+command.first());
            ST_PRINTLINE(F(": command not found"));
        }

        return found;
      }

      /**
       * Number of commands that have been defined
       */
      uint8_t size() {
        return this->size_ + 1;
      }

      /**
       * loop() : should be called every execution cycle to process characters entered  
       */
      void loop() {
        #ifndef ST_FLAG_NOHELP
        if (this->firstRun) {
          this->firstRun = false;
          this->printCommands();
          displayPrompt();
        }
        #endif

        if (!Serial.available()) return;

        bool commandComplete = false;
        while (Serial.available()) {
            char car = Serial.read();
            if (isAscii(car)) {
                Serial.print(car);
            }

            if (car == 127 && this->message.length() > 0) {
                Serial.print("\e[1D");
                Serial.print(' ');
                Serial.print("\e[1D");
                this->message.remove(this->message.length() - 1);
                continue;
            }
           
            // Check if user ended the line
            if (car == '\r') {
                Serial.print(CRLF);
                commandComplete = true;
                // If there are more data on the line, drop a \n, if it is
                // there. Some terminals may send both, giving 
                // an extra lineend, if we do not drop it.
                if (Serial.available() && Serial.peek() == '\n') {
                    Serial.read();
                }
                Serial.flush();
                Serial.read();
                break;
            }
            this->message += car;
        }
      
        // If we have a messge then look it up and call the action
        if (commandComplete && !this->message.isEmpty()) {
          // Lookup the action for the message and call it
          bool found = findCommandAndCallAction( this->message);
          this->message = "";
        }
        
        // Is the message
        if( this->message.isEmpty()) {
          displayPrompt();
        }

      }

      static Pair<String, String> ParseCommand(String message) {
        String keyword = "";
        for (auto& car : message) {
          if (car == ' ') break;
          keyword += car;
        }
        if (!keyword.isEmpty()) {
          message.remove(0, keyword.length());
        }

        keyword.trim();
        message.trim();

        return Pair<String, String>(keyword, message);
      }

      /**
       * Display the prompt to the console if required  
       */
      void displayPrompt(){
        #ifndef ST_FLAG_NOPROMPT
        ST_PRINT(CRLF + prompt + "> ");
        #endif
      }

      /**
       * Parse the entered string
      */
      static String ParseArgument(String message) {
        String keyword;
        for (auto& car : message) {
          if (car == '"')
            break;
          keyword += car;
        }
        if (!keyword.isEmpty())
        message.remove(0, keyword.length());
        message.trim();
        int msg_len = message.length();
        if (msg_len > 0) {
            message.remove(0,1);
            message.remove(msg_len-2);
        }

        return message;
      }
  };

}
