// ErrorHandler.h - Error Message Handling
#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <M5Dial.h>
#include "config.h"

// Parse server error response and return display message
String getErrorDisplayMessage(String serverResponse) {
    // Check against known error mappings
    for (int i = 0; i < ERROR_MESSAGES_COUNT; i++) {
        if (serverResponse.indexOf(ERROR_MESSAGES[i].serverMessage) >= 0) {
            return ERROR_MESSAGES[i].displayMessage;
        }
    }
    // Default error message if no match found
    return "SERVER\nERROR";
}

// Display error message on red background
void displayErrorMessage(String errorMessage) {
    M5Dial.Display.fillScreen(RED);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString(errorMessage, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
    delay(FAIL_MESSAGE_DELAY);
}

#endif // ERROR_HANDLER_H
