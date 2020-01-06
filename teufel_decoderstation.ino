#include <IRremote.h>
#include <Bounce2.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EasyWebServer.h>

// IR receiver
#define RECEIVE_PIN 9
IRrecv irrecv(RECEIVE_PIN);
decode_results results;

// IR sender
#define SEND_PIN 3
IRsend irsend;

// IR carrier frequency
#define IR_KHZ 38

// IR repeat signal
const unsigned int irRepeat[] = {8900, 2200, 550};

// debouncing the button
#define BUTTON_PIN 5
Bounce debouncer = Bounce();

// ethernet configuration
byte mac_address[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip_address(192, 168, 178, 3);

// listen on tcp port 80
EthernetServer server(80);

// timestamp at which last user interaction occurred (milliseconds)
unsigned long lastInteraction = 0;

// states of the remote
#define STATE_WAITING 0  // waiting for the next key press
#define STATE_HOLDING 1  // holding the key
#define STATE_EXECUTE 2  // execute the command according to the current count
#define STATE_REPEAT 3   // repeat the last command
unsigned long curState = STATE_WAITING;

// count the key presses to select a command
unsigned long curCount = 0;

// from IR example for dumping the received codes
void ir_dump(decode_results *results) {
    // Dumps out the decode_results structure.
    // Call this after IRrecv::decode()
    int count = results->rawlen;
    if (results->decode_type == UNKNOWN) {
        Serial.print("Unknown encoding: ");
    } else if (results->decode_type == NEC) {
        Serial.print("Decoded NEC: ");

    } else if (results->decode_type == SONY) {
        Serial.print("Decoded SONY: ");
    } else if (results->decode_type == RC5) {
        Serial.print("Decoded RC5: ");
    } else if (results->decode_type == RC6) {
        Serial.print("Decoded RC6: ");
    } else if (results->decode_type == PANASONIC) {
        Serial.print("Decoded PANASONIC - Address: ");
        Serial.print(results->address, HEX);
        Serial.print(" Value: ");
    } else if (results->decode_type == LG) {
        Serial.print("Decoded LG: ");
    } else if (results->decode_type == JVC) {
        Serial.print("Decoded JVC: ");
    } else if (results->decode_type == AIWA_RC_T501) {
        Serial.print("Decoded AIWA RC T501: ");
    } else if (results->decode_type == WHYNTER) {
        Serial.print("Decoded Whynter: ");
    }
    Serial.print(results->value, HEX);
    Serial.print(" (");
    Serial.print(results->bits, DEC);
    Serial.println(" bits)");
    /*Serial.print("Raw (");
    Serial.print(count, DEC);
    Serial.print("): ");

    for (int i = 1; i < count; i++) {
        if (i & 1) {
        Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
        }
        else {
        Serial.write('-');
        Serial.print((unsigned long) results->rawbuf[i]*USECPERTICK, DEC);
        }
        Serial.print(" ");
    }
    Serial.println();*/
}

void web_root(EasyWebServer& ews) {
    ews.client.println(F("<!DOCTYPE HTML>"));
    ews.client.println(F("<html><head><title>Teufel Decoderstation 5 Remote</title></head><body>"));
    ews.client.println(F("<p>Teufel Decoderstation 5 Remote:</p>"));
    ews.client.println(F("<p><a href='/repeat3'>Repeat 3 Times</a></p>"));
    ews.client.println(F("<p><a href='/repeat10'>Repeat 10 Times</a></p>"));
    ews.client.println(F("<p><a href='/vol_decr'>Decrease Volume</a></p>"));
    ews.client.println(F("<p><a href='/vol_incr'>Increase Volume</a></p>"));
    ews.client.println(F("<p><a href='/mute'>Mute/Unmute</a></p>"));
    ews.client.println(F("</body></html>"));
}

void web_repeat3(EasyWebServer& ews) {
    // easy web server doesn't send a newline
    Serial.println("");

    // send the repeat signal
    for (int i = 0; i < 3; ++i) {
        Serial.println("IR Repeat!");
        irsend.sendRaw(irRepeat, sizeof(irRepeat) / sizeof(irRepeat[0]), IR_KHZ);
        delay(95); // delay the IR repeat signal
    }

    // show the root page
    web_root(ews);
}

void web_repeat10(EasyWebServer& ews) {
    // easy web server doesn't send a newline
    Serial.println("");

    // send the repeat signal
    for (int i = 0; i < 10; ++i) {
        Serial.println("IR Repeat!");
        irsend.sendRaw(irRepeat, sizeof(irRepeat) / sizeof(irRepeat[0]), IR_KHZ);
        delay(95); // delay the IR repeat signal
    }

    // show the root page
    web_root(ews);
}

void web_vol_decr(EasyWebServer& ews) {
    // easy web server doesn't send a newline
    Serial.println("");

    // send IR command to decrease the volume
    Serial.println("IR volume decrease!");
    irsend.sendNEC(0x807F6A95, 32);

    // show the root page
    web_root(ews);
}

void web_vol_incr(EasyWebServer& ews) {
    // easy web server doesn't send a newline
    Serial.println("");

    // send IR command to increase the volume
    Serial.println("IR volume increase!");
    irsend.sendNEC(0x807F7A85, 32);

    // show the root page
    web_root(ews);
}

void web_mute(EasyWebServer& ews) {
    // easy web server doesn't send a newline
    Serial.println("");

    // send IR command to mute
    Serial.println("IR mute!");
    irsend.sendNEC(0x807FD02F, 32);

    // show the root page
    web_root(ews);
}

void setup() {
    // initialize serial connection
    Serial.begin(9600);

    // intialize the IR receiver
    irrecv.enableIRIn();

    // initialize button
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    debouncer.attach(BUTTON_PIN);
    debouncer.interval(5);

    // initialize the ethernet server
    Ethernet.begin(mac_address, ip_address);
    server.begin();
}

void loop() {
    // IR receiver and display
    if (irrecv.decode(&results)) {
        // Serial.println(results.value, HEX);
        ir_dump(&results);
        irrecv.resume();  // Receive the next value
    }

    // debounce
    debouncer.update();

    // current time
    const unsigned long cur_time = millis();

    // time difference to the last interaction
    const unsigned long time_diff = cur_time - lastInteraction;

    // count the number of key presses and execute the command if the user holds the key
    if (curState == STATE_WAITING && debouncer.fell()) {
        // user just pressed the button
        Serial.println("Transition to STATE_HOLDING.");
        curState = STATE_HOLDING;
        lastInteraction = cur_time;
        return;
    } else if (curState == STATE_HOLDING && debouncer.rose()) {
        // user just released the button
        Serial.println("Transition to STATE_WAITING.");
        curState = STATE_WAITING;
        curCount++;
        lastInteraction = cur_time;
        return;
    } else if (curState == STATE_HOLDING && !debouncer.rose() && time_diff > 150) {
        // user wants to execute a command
        Serial.println("Transition to STATE_EXECUTE.");
        curState = STATE_EXECUTE;
    }

    // execute the command according to the current key press count
    if (curState == STATE_EXECUTE) {
        Serial.print(curCount, DEC);
        Serial.println(" is the key press count.");
        switch (curCount) {
            case 0:
                Serial.println("IR volume decrease!");
                irsend.sendNEC(0x807F6A95, 32);
                break;
            case 1:
                Serial.println("IR volume increase!");
                irsend.sendNEC(0x807F7A85, 32);
                break;
        }
        Serial.println("Transition to STATE_REPEAT.");
        curState = STATE_REPEAT;
        lastInteraction = cur_time;
        delay(95);  // delay the IR repeat signal
    }

    // send the repeat signal or restart the key press counting
    if (curState == STATE_REPEAT) {
        if (!debouncer.rose()) {
            Serial.println("IR Repeat!");
            irsend.sendRaw(irRepeat, sizeof(irRepeat) / sizeof(irRepeat[0]), IR_KHZ);
            lastInteraction = cur_time;
            delay(95);  // delay the IR repeat signal
        } else {
            Serial.println("Transition to STATE_WAITING.");
            curState = STATE_WAITING;
            curCount = 0;
        }
    }

    // fall back to the initial state if there was no interaction for one second
    if (curState != STATE_WAITING && curState != STATE_REPEAT && time_diff > 1000) {
        Serial.println("Transition to STATE_WAITING.");
        curState = STATE_WAITING;
        curCount = 0;
    }

    // check for http client
    EthernetClient client = server.available();
    if (client) {
        Serial.println("New HTTP client.");
        EasyWebServer ews(client);
        ews.serveUrl("/", web_root);
        ews.serveUrl("/repeat3", web_repeat3);
        ews.serveUrl("/repeat10", web_repeat10);
        ews.serveUrl("/vol_decr", web_vol_decr);
        ews.serveUrl("/vol_incr", web_vol_incr);
        ews.serveUrl("/mute", web_mute);
    }
}
