/* #defines and typedefs */
#define BAUD_RATE            57600
#define COMMAND_MAX_LENGTH   15
#define DEFAULT_HIGH_TIME_US 1000000
#define DEFAULT_LOW_TIME_US  DEFAULT_HIGH_TIME_US

// using these defines for faster pin access witout indirection
#define TARGET_PORT          PORTD
#define TARGET_PIN           3
#define TARGET_ARDUINO_PIN   3

typedef enum{
  high_time,
  low_time,
  stat,
  help,
  cmd_empty,
  cmd_incomplete,
  cmd_invalid
} command_t;

/* function prototypes */
command_t get_command(char chr);
void clear_command();
void append_to_command(char chr);
boolean is_alpha_numeric(char chr);
command_t parse_command();  
void backspace();
void cursor_left();
void high_time_cmd(uint32_t time_us);
void low_time_cmd(uint32_t time_us);
void stat_cmd();
void help_cmd();
void prompt();


/* global variables */
char command[COMMAND_MAX_LENGTH + 1];
uint8_t  command_write_index = 0;
uint32_t command_argument = 0;

long high_time_us = DEFAULT_HIGH_TIME_US; 
long low_time_us = DEFAULT_LOW_TIME_US;
uint8_t  target_pin  = 3;

int pin_state = LOW;        
long previous_us = 0;
long interval_us = high_time_us;

/* arduino functions */
void setup(){
  Serial.begin(BAUD_RATE);      
  pinMode(TARGET_ARDUINO_PIN, OUTPUT);
  clear_command();
  help_cmd();
  stat_cmd();
  prompt();
}

void loop(){
  char chr;
  // process commands from the serial terminal
  while(Serial.available() > 0){
    chr = Serial.read();
    //Serial.print("Code: ");
    //Serial.println(chr, HEX);
    switch(get_command(chr)){
    case high_time:
      high_time_cmd(command_argument);
      Serial.println(F("ok"));      
      prompt();
      break;
    case low_time:
      low_time_cmd(command_argument);
      Serial.println(F("ok"));  
      prompt();      
      break;
    case stat:
      stat_cmd();
      prompt();      
      break;
    case cmd_empty:
      prompt();
      break;
    case help:
    case cmd_invalid:    
      Serial.print(F("Unrecognized Command: "));
      Serial.println(command);
      help_cmd();
      prompt();      
      break;
    }     
    
    if(chr == '\r' || chr == '\n'){
      clear_command();       
    }
  }
  
  // run the pin toggling state machine 
  unsigned long current_us = micros();
  if(current_us - previous_us > interval_us) {
    previous_us = current_us;
    if (pin_state == LOW){
      TARGET_PORT |= _BV(TARGET_PIN);
      //Serial.println("HIGH");
      pin_state = HIGH;
      interval_us = high_time_us;      
    }
    else{
      TARGET_PORT &= ~_BV(TARGET_PIN);      
      //Serial.println("LOW");
      pin_state = LOW;      
      interval_us = low_time_us;      
    }
  }
  
}

/* function implementations */
command_t get_command(char chr){  
  // handle backspace / delete
  if(chr == 0x08 || chr == 0x7F){    
    backspace();  
    return cmd_incomplete;    
  }
  // newline signals the need to process the command
  else if(chr == '\r'){
    Serial.println(); // echo the newline
    return parse_command(); // parse the command
  }  
  // ignore whitespace and non-alphanumeric characters 
  else if(chr == ' ' || chr == '\n' || !is_alpha_numeric(chr)){
    Serial.print(' ');
    return cmd_incomplete;
  }
  else{
    // echo the character
    Serial.print(chr); // echo the character
    append_to_command(chr); // add the character to the command
    return cmd_incomplete;    
  }
}

void clear_command(){
  memset(command, 0, COMMAND_MAX_LENGTH + 1); 
  command_write_index = 0;
  command_argument = 0;
}

boolean is_alpha_numeric(char chr){
  if(chr >= 'a' && chr <= 'z'){
    return true;
  } 
  else if(chr >= 'A' && chr <= 'Z'){
    return true; 
  }
  else if(chr >= '0' && chr <= '9'){
    return true; 
  }
  else{
    return false; 
  }
}

command_t parse_command(){
  if(strncmp_P(command, PSTR("help"), 4) == 0){
    return help; 
  }
  
  if(strlen(command) == 0){
    return cmd_empty; 
  }
  
  // parse the argument as a 32-bit integer if it is possible
  char * endptr = (char *) 0;
  if(strlen(command) > 1){
    command_argument = strtol(command+1, &endptr, 10);
  }
  
  switch(command[0]){
  case 's':
    return stat;
  case 'h':    
    if((*endptr == '\0') && (command_argument > 0)){
      return high_time;
    }
    break;
  case 'l':
    if((*endptr == '\0') && (command_argument > 0)){
      return low_time;
    }
    break;
  }  
  
  return cmd_invalid;
}

void append_to_command(char chr){
  // convert to lower case  
  if(chr >= 'A' && chr <= 'Z'){
    chr = chr - 'A' + 'a';
  }
  
  // add it to the buffer if there's room
  // and advance the write index 
  if(command_write_index <= COMMAND_MAX_LENGTH){
    command[command_write_index++] = chr;
  } 
}

void cursor_left(){
  Serial.write(0x1B);
  Serial.write(0x5B);
  Serial.write(0x44);
}

void backspace(){
  if(command_write_index > 0){
    cursor_left();
    Serial.print(' '); // emit a space to clear the displayed character
    cursor_left();
    command_write_index--;
  }
  command[command_write_index] = 0; // erase the character
}

void high_time_cmd(uint32_t time_us){
  high_time_us = time_us;
}

void low_time_cmd(uint32_t time_us){
  low_time_us = time_us;  
}

void stat_cmd(){
  Serial.println(F("Current Parameter Values"));
  Serial.println(F("===================================================="));
  Serial.print(F("  High Time: "));
  Serial.println(high_time_us);
  Serial.print(F("  Low Time: "));
  Serial.println(low_time_us);
  Serial.println();
}

void help_cmd(){
  Serial.println(F("Recognized Commands"));
  Serial.println(F("===================================================="));
  Serial.println(F("  h time_us"));
  Serial.println(F("    set the time the signal is high in microseconds"));
  Serial.println(F("  l time_us"));
  Serial.println(F("    set the time the signal is low in microseconds"));
  Serial.println(F("  s"));
  Serial.println(F("    display the current parameter values"));
  Serial.println(F("  help"));
  Serial.println(F("    display this help menu"));
  Serial.println();  
}

void prompt(){
  Serial.print(F(">")); 
}
