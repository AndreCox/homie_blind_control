// Adapted from http://www.markisolgroup.com/en/products/ifit.html by Sillyfrog
/*
******************************************************************************************************************************************************************
*
* Markisol iFit Spring Pro 433.92MHz window shades
* Also sold under the name Feelstyle and various Chinese brands like Bofu
* Compatible with remotes like BF-101, BF-301, BF-305, possibly others
*
* Code by Antti Kirjavainen (antti.kirjavainen [_at_] gmail.com)
*
* http://www.markisolgroup.com/en/products/ifit.html
*
* Unless I'm completely mistaken, each remote has its unique, hard coded ID. I've included all commands from one as an
* example, but you can use RemoteCapture.ino to capture your own remotes. The purpose of this project was to get my own
* window shades automated, so there's a bit more work to be done, to reverse engineer the final 9 trailing bits of the
* commands. I don't (yet) know how they're formed, but most motors simply seem to ignore them. However, some require
* them to be correct and sendShortMarkisolCommand() will not work with such motors (use the full 41 bit command with
* sendMarkisolCommand() in that case).
*
* Special thanks to Chris Teel for testing timings with some Rollerhouse products to achieve better compatibility.
*
*
* HOW TO USE
*
* Capture your remote controls with RemoteCapture.ino and copy paste the 41 bit commands to Markisol.ino for sendMarkisolCommand().
* More info about this provided in RemoteCapture.ino.
*
* If you have no multichannel remotes like the BF-305, you can also try the sendShortMarkisolCommand() function with
* your 16 bit remote ID and a command (see setup() below for more info). This function will not work with every motor,
* so first try the full 41 bit commands.
*
*
* HOW TO USE WITH EXAMPLE COMMANDS
*
* 1. Set the shade into pairing mode by holding down its P/SETTING button until it shakes twice ("TA-TA") or beeps.
* 2. Send the pairing command, eg. "sendMarkisolCommand(SHADE_PAIR_EXAMPLE);", which will shake the shade twice ("TA-TA") or beep.
* 3. Now you can control the shade, eg. sendMarkisolCommand(SHADE_DOWN_EXAMPLE); (or SHADE_UP_EXAMPLE, SHADE_STOP_EXAMPLE etc.).
*
* Setting limits is quicker with the remotes, although you can use your Arduino for that as well. Some motors do not erase the
* the limits even if you reset them by holding down the P/SETTING button for 8-10 seconds. They just reset the list of registered
* remote controls.
*
*
* PROTOCOL DESCRIPTION
*
* Tri-state bits are used.
* A single command is: 3 AGC bits + 41 command tribits + radio silence
*
* All sample counts below listed with a sample rate of 44100 Hz (sample count / 44100 = microseconds).
*
* Starting (AGC) bits:
* HIGH of approx. 216 samples = 4885 us
* LOW of approx. 108 samples = 2450 us
* HIGH of approx. 75 samples = 1700 us
*
* Pulse length:
* SHORT: approx. 15 samples = 340 us
* LONG: approx. 30 samples = 680 us
*
* Data bits:
* Data 0 = short LOW, short HIGH, short LOW (wire 010)
* Data 1 = short LOW, long HIGH (wire 011)
*
* Command is as follows:
* 16 bits for (unique) remote control ID, hard coded in remotes
* 4 bits for channel ID: 1 = 1000 (also used by BF-301), 2 = 0100 (also used by BF-101), 3 = 1100, 4 = 0010, 5 = 1010, ALL = 1111
* 4 bits for command: DOWN = 1000, UP = 0011, STOP = 1010, CONFIRM/PAIR = 0010, LIMITS = 0100, ROTATION DIRECTION = 0001
* 8 bits for remote control model: BF-305 multi = 10000110, BF-101 single = 00000011, BF-301 single = 10000011
* 9 bits for something? (most likely some type of checksum)
*
* = 41 bits in total
*
* There is a short LOW drop of 80 us at the end of each command, before the next AGC (or radio silence at the end of last command).
* End the last command in sequence with LOW radio silence of 223 samples = 5057 us
*
*
* HOW THIS WAS STARTED
*
* Project started with a "poor man's oscilloscope": 433.92MHz receiver unit (data pin) -> 10K Ohm resistor -> USB sound card line-in.
* Try that at your own risk. Power to the 433.92MHz receiver unit was provided by Arduino (connected to 5V and GND).
*
* To view the waveform Arduino is transmitting (and debugging timings etc.), I found it easiest to directly connect the digital pin (13)
* from Arduino -> 10K Ohm resistor -> USB sound card line-in. This way the waveform was very clear.
*
* Note that a PC sound cards may capture the waveform "upside down" (phase inverted). You may need to apply Audacity's Effects -> Invert
* to get the HIGHs and LOWs correctly.
*
******************************************************************************************************************************************************************
*/
#include <Arduino.h>

/*
Samples from RemoteCapture (and reformatted)

Successful capture, full command is: 
01110100100011111000001110000011000001101

Bits 0-15 (16 long): 0111010010001111  Hex: 0x748F
Remote control (unique) ID: 0x748F

Bits 16-19 (4 long): 1000  Hex: 0x8
Channel: 1

Bits 20-23 (4 long): 0011  Hex: 0x3
Command: UP

Bits 24-31 (8 long): 10000011  Hex: 0x83
Remote control model: BF-301 (single channel)

Bits 32-40 (9 long): 000001101  Hex: 0xD
Trailing bits: 13



Successful capture, full command is: 11111101010010101010001110000000010101001

Bits 0-15 (16 long): 1111110101001010  Hex: 0xFD4A
Remote control (unique) ID: 0xFD4A

Bits 16-19 (4 long): 1010  Hex: 0xA
Channel: 5

Bits 20-23 (4 long): 0011  Hex: 0x3
Command: UP

Bits 24-31 (8 long): 10000000  Hex: 0x80
Remote control model: UNKNOWN/NEW

Bits 32-40 (9 long): 010101001  Hex: 0xA9
Trailing bits: 169

*/

// Example commands to try (or just capture your own remotes with RemoteCapture.ino):
#define SHADE_PAIR_EXAMPLE "10111011111011111000001010000011110101001"             // C button
#define SHADE_DOWN_EXAMPLE "10111011111011111000100010000011110110101"             // DOWN button
#define SHADE_STOP_EXAMPLE "10111011111011111000101010000011110110001"             // STOP button
#define SHADE_UP_EXAMPLE "10111011111011111000001110000011110101011"               // UP button
#define SHADE_LIMIT_EXAMPLE "10111011111011111000010010000011110100101"            // L button
#define SHADE_CHANGE_DIRECTION_EXAMPLE "10111011111011111000000110000011110101111" // STOP + L buttons

#define TRANSMIT_PIN D5  // GPIO14
#define REPEAT_COMMAND 8 // How many times to repeat the same command: original remotes repeat 8 (multi) or 10 (single) times by default
#define DEBUG false      // Do note that if you print serial output during transmit, it will cause delay and commands may fail

#define FULL_COMAND_TEMPLATE 0b00000000000000000000000010000011000000000 // Default command template, with the correct bits to be set as required

// For sendShortMarkisolCommand():
//#define MY_REMOTE_ID_1                    "0000000000000000"    // Enter your 16 bit remote ID here or make up a binary number and confirm/pair it with the motor
#define DEFAULT_CHANNEL 0b1000                   // Channel information is only required for multichannel remotes like the BF-305
#define DEFAULT_REMOTE_MODEL 0b10000011          // We default to BF-301, but this is actually ignored by motors and could be plain zeroes
#define DEFAULT_TRAILING_BITS 0b000000000        // Last 9 bits of the command. What do they mean? No idea. Again, ignored by motors.
#define COMMAND_DOWN 0b1000                      // Remote button DOWN
#define COMMAND_UP 0b0011                        // Remote button UP
#define COMMAND_STOP 0b1010                      // Remote button STOP
#define COMMAND_PAIR 0b0010                      // Remote button C
#define COMMAND_PROGRAM_LIMITS 0b0100            // Remote button L
#define COMMAND_CHANGE_ROTATION_DIRECTION 0b0001 // Remote buttons STOP + L

// Timings in microseconds (us). Get sample count by zooming all the way in to the waveform with Audacity.
// Calculate microseconds with: (samples / sample rate, usually 44100 or 48000) - ~15-20 to compensate for delayMicroseconds overhead.
// Sample counts listed below with a sample rate of 44100 Hz:
#define MARKISOL_AGC1_PULSE 4885    // 216 samples
#define MARKISOL_AGC2_PULSE 2450    // 108 samples
#define MARKISOL_AGC3_PULSE 1700    // 75 samples
#define MARKISOL_RADIO_SILENCE 5057 // 223 samples

#define MARKISOL_PULSE_SHORT 340 // 15 samples
#define MARKISOL_PULSE_LONG 680  // 30 samples

#define MARKISOL_COMMAND_BIT_ARRAY_SIZE 41 // Command bit count

// NOTE: If you're having issues getting the motors to respond, try these previous defaults:
//#define MARKISOL_AGC2_PULSE                   2410  // 107 samples
//#define MARKISOL_AGC3_PULSE                   1320  // 59 samples
//#define MARKISOL_PULSE_SHORT                  300   // 13 samples

// This is true while a signal gets broadcast.
bool sending = false;

#define RECENT_REMOTE_MAX 10
uint16_t recent_remote_ids[RECENT_REMOTE_MAX];
int recent_remote_count = 0;

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void errorLog(String message)
{
  Serial.println(message);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// Adds the given remote to teh list of recent_remote_ids if it's not there already
// taking care of rotating etc if the array is full
void logRemoteId(uint16_t remote_id)
{
  for (int i = 0; i < recent_remote_count; i++)
  {
    if (recent_remote_ids[i] == remote_id)
    {
      return;
    }
  }
  if (recent_remote_count >= RECENT_REMOTE_MAX)
  {
    // Shuffle everything along one to make room for this remote
    for (int i = 0; i < RECENT_REMOTE_MAX - 1; i++)
    {
      recent_remote_ids[i] = recent_remote_ids[i + 1];
    }
  }
  else
  {
    recent_remote_count += 1;
  }
  recent_remote_ids[recent_remote_count - 1] = remote_id;
}

// Swaps the lower 4 bits (big vs little endian)
uint8_t swap4bits(uint8_t bits)
{
  uint8_t ret = 0;
  if (bits & 0b1000)
    ret += 0b0001;
  if (bits & 0b0100)
    ret += 0b0010;
  if (bits & 0b0010)
    ret += 0b0100;
  if (bits & 0b0001)
    ret += 0b1000;
  return ret;
}

// Read a passed sample array, and attempt to decode it
uint64_t readSample(uint16_t samples[], int samplecount)
{
  if (samplecount < 86)
  {
    Serial.println("Not enough samples, ignoring");
    return 0;
  }

  // Start with the headers
  uint64_t command = 0;
  if (samples[1] < 2300 || samples[1] > 2600)
  {
    // Low
    Serial.println("Broke at 1");
    return 0;
  }
  if (samples[2] < 1100 || samples[2] > 1900)
  {
    // High
    Serial.println("Broke at 2");
    return 0;
  }
  // So far so good check bits...
  uint16 i = 4;
  while (i < 86)
  {
    uint16_t t = samples[i];
    command = command << 1;

    if (t > 500 && t < 800)
    { // Found 1
      // command += "1";
      // Serial.print("1");
      command += 1;
    }
    else if (t > 200 && t < 400)
    { // Found 0
      // command += "0";
      // Serial.print("0");
    }
    else
    { // Unrecognized bit, finish
      Serial.print("INVALID TIMING: ");
      Serial.println(t);
      return 0;
    }

    i += 2;
  }
  return command;
}

String commandToString(uint8_t c)
{
  switch (c)
  {
  case 0x8: // 1000
    return "DOWN";
  case 0x3: // 0011
    return "UP";
  case 0xA: // 1010
    return "STOP";
  case 0x2: // 0010
    return "PAIR";
  case 0x4: // 0100
    return "LIMIT";
  case 0x1: // 0001
    return "DIRECTION";
  }
  return "";
}

// Returns a string representation of the command that could be given to sendStringCommand
String fullCommandToString(uint64_t command)
{
  uint64_t working = command >> 17;
  working = working & 0xf;
  String output = commandToString((uint8_t)working);
  if (output.length() == 0)
  {
    return "";
  }

  working = command >> 25;
  uint16_t remote_id = working;
  logRemoteId(remote_id);
  String remote_id_str = String(remote_id, 16);

  output += "," + remote_id_str;

  working = command >> 21;
  working = working & 0xf;
  uint8_t channel = (uint8_t)working;
  channel = swap4bits(channel);
  if (channel == 0xf)
    channel = 0;

  output += "," + String(channel);
  return output;
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitHigh(int delay_microseconds)
{
  digitalWrite(TRANSMIT_PIN, HIGH);
  delayMicroseconds(delay_microseconds);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void transmitLow(int delay_microseconds)
{
  digitalWrite(TRANSMIT_PIN, LOW);
  delayMicroseconds(delay_microseconds);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

String uint64ToString(uint64_t input, uint8_t base)
{
  String result = "";
  // prevent issues if called with base <= 1
  if (base < 2)
    base = 10;
  // Check we have a base that we can actually print.
  // i.e. [0-9A-Z] == 36
  if (base > 36)
    base = 10;

  // Reserve some string space to reduce fragmentation.
  // 16 bytes should store a uint64 in hex text which is the likely worst case.
  // 64 bytes would be the worst case (base 2).
  result.reserve(16);

  do
  {
    char c = input % base;
    input /= base;

    if (c < 10)
      c += '0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}

void setupMarkisol()
{
  pinMode(TRANSMIT_PIN, OUTPUT); // Prepare the digital pin for output
  digitalWrite(TRANSMIT_PIN, LOW);
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void doMarkisolTribitSend(uint64_t command)
{
  sending = true;
  // Starting (AGC) bits:
  transmitHigh(MARKISOL_AGC1_PULSE);
  transmitLow(MARKISOL_AGC2_PULSE);
  transmitHigh(MARKISOL_AGC3_PULSE);

  // Transmit command:
  for (int i = 0; i < MARKISOL_COMMAND_BIT_ARRAY_SIZE; i++)
  {
    // Bit shift right only as uint64 has bugs left shifting over 32 bits
    uint64_t mask = 0x10000000000 >> i;
    if (command & mask)
    {
      // If current bit is 1, transmit LOW-HIGH-HIGH (011):
      transmitLow(MARKISOL_PULSE_SHORT);
      transmitHigh(MARKISOL_PULSE_LONG);
    }
    else
    {
      // If current bit is 0, transmit LOW-HIGH-LOW (010):
      transmitLow(MARKISOL_PULSE_SHORT);
      transmitHigh(MARKISOL_PULSE_SHORT);
      transmitLow(MARKISOL_PULSE_SHORT);
    }
  }
  sending = false;

  if (DEBUG)
  {
    Serial.println();
    Serial.print("Transmitted ");
    Serial.print(MARKISOL_COMMAND_BIT_ARRAY_SIZE);
    Serial.println(" bits.");
    Serial.println();
  }
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------
void sendMarkisolCommand(uint64_t command)
{
  if (command == 0)
  {
    errorLog("sendMarkisolCommand(): Command was NULL, cannot continue.");
    return;
  }

  // Repeat the command:
  for (int i = 0; i < REPEAT_COMMAND; i++)
  {
    doMarkisolTribitSend(command);
  }

  // Radio silence at the end of last command.
  // It's better to go a bit over than under minimum required length:
  transmitLow(MARKISOL_RADIO_SILENCE);

  // Disable output to transmitter to prevent interference with
  // other devices. Otherwise the transmitter will keep on transmitting,
  // disrupting most appliances operating on the 433.92MHz band:
  digitalWrite(TRANSMIT_PIN, LOW);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

uint64_t partsToMarkisolCommand(uint8_t command, uint16_t remote_id, uint8_t channel)
{
  logRemoteId(remote_id);
  // No left bit shifting due to bugs over the 32 bit boundary.
  uint64_t remote_id64 = (uint64_t)remote_id * 0b10000000000000000000000000;
  uint64_t channel64 = (uint64_t)channel * 0b1000000000000000000000;
  uint64_t command64 = (uint64_t)command * 0b100000000000000000;

  // Let's form and transmit the full command:
  uint64_t full_command = FULL_COMAND_TEMPLATE + remote_id64 + channel64 + command64;

  return full_command;
}

void sendShortMarkisolCommand(uint8_t command, uint16_t remote_id, uint8_t channel)
{
  sendMarkisolCommand(partsToMarkisolCommand(command, remote_id, channel));
}

/*
Convert a command that's given as a string in the format:
<command>,<remote id>[,<channel>]
command: a string, one of up,down,stop,pair,limit,direction
remote id: The ID of the remote as a hex string (16 bit)
channel: Optional channel - defaults to 1, if 0, means "all channels"
Returns the binary representation of the actual command.
*/
uint64_t stringCommandToCommand(String fullCommand)
{
  int firstSplit = fullCommand.indexOf(",");
  int nextSplit = fullCommand.indexOf(",", firstSplit + 1);
  String commandstr;
  String remotestr;
  String channelstr = "1";
  if (nextSplit < 0)
  {
    commandstr = fullCommand.substring(0, firstSplit);
    remotestr = fullCommand.substring(firstSplit + 1);
  }
  else
  {
    commandstr = fullCommand.substring(0, firstSplit);
    remotestr = fullCommand.substring(firstSplit + 1, nextSplit);
    channelstr = fullCommand.substring(nextSplit + 1);
  }
  uint8_t command = 0;
  if (commandstr.equalsIgnoreCase("down"))
    command = COMMAND_DOWN;
  else if (commandstr.equalsIgnoreCase("up"))
    command = COMMAND_UP;
  else if (commandstr.equalsIgnoreCase("stop"))
    command = COMMAND_STOP;
  else if (commandstr.equalsIgnoreCase("pair"))
    command = COMMAND_PAIR;
  else if (commandstr.equalsIgnoreCase("limit"))
    command = COMMAND_PROGRAM_LIMITS;
  else if (commandstr.equalsIgnoreCase("direction"))
    command = COMMAND_CHANGE_ROTATION_DIRECTION;
  else
  {
    Serial.println("Invalid command, aborting");
    return 0; // Invalid command, abort
  }

  uint16_t remote_id = strtoul(remotestr.c_str(), NULL, 16);
  uint8_t channel = atoi(channelstr.c_str());

  if (channel == 0)
  {
    // Special handling for the zero case
    channel = 0xF;
  }
  else
  {
    if (channel >= 0xf)
    {
      // Invalid channel, abort
      Serial.println("Invalid channel, aborting");
      return 0;
    }
    // The channel bits must be flipped (little vs big endian)
    channel = swap4bits(channel);
  }

  Serial.printf("Final values, Remote ID: %x, Command: %d  Channel: %d\n", remote_id, command, channel);
  return partsToMarkisolCommand(command, remote_id, channel);
}

/*
Send a command that's given as a string in the format:
<command>,<remote id>[,<channel>]
command: a string, one of up,down,stop,pair,limit,direction
remote id: The ID of the remote as a hex string (16 bit)
channel: Optional channel - defaults to 1, if 0, means "all channels"
*/
void sendStringCommand(String fullCommand)
{
  uint64_t commandbinary = stringCommandToCommand(fullCommand);
  if (commandbinary)
  {
    sendMarkisolCommand(commandbinary);
  }
}