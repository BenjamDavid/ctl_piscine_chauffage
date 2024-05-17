#include <Wire.h>
#include <LiquidCrystal_I2C.h>



#include <OneWire.h>

const byte PIN_BUTTON_OK = 5;
const byte PIN_BUTTON_BACK = 4;
const byte PIN_BUTTON_PLUS = 3;
const byte PIN_BUTTON_MINUS = 2;

const byte PIN_SENSOR_1 = 6;
const byte PIN_SENSOR_2 = 7;
const byte PIN_SENSOR_3 = 8;

/* Code de retour de la fonction getTemperature() */
enum DS18B20_RCODES {
  NO_SENSOR_FOUND,
  SENSOR_FOUND,
  READ_OK,  // Lecture ok
  INVALID_ADDRESS,  // Adresse reçue invalide
  INVALID_SENSOR  // Capteur invalide (pas un DS18B20)
};


/* Création de l'objet OneWire pour manipuler le bus 1-Wire */
OneWire ds1(PIN_SENSOR_1);
OneWire ds2(PIN_SENSOR_2);
OneWire ds3(PIN_SENSOR_3);
 

//// cablage  GND sur gnd 
//            VCC sur 5V
//            SDA sur A4
//            SCL sur A5

LiquidCrystal_I2C lcd(0x3F,16,2);

#define NB_SENSOR (3)
typedef struct temp {
  OneWire* ds;
  byte addr[8];
  DS18B20_RCODES state;
  float temperature;
  char name[4];  
  float order;
  float hysteresis;
}t_temp;

byte g_nb_sensor = 0;
t_temp g_temp[NB_SENSOR]={{&ds1,{0,0,0,0,0,0,0,0},NO_SENSOR_FOUND,0.0,"air",22.0,1.0},
                           {&ds2,{0,0,0,0,0,0,0,0},NO_SENSOR_FOUND,0.0,"in",10.0,1.0},
                           {&ds3,{0,0,0,0,0,0,0,0},NO_SENSOR_FOUND,0.0,"out",20.0,1.0}};


typedef void (*Execute)(void *);
#define READING_WAIT_TIME (8) /* in 100ms => 800ms*/
void TemperatureStartRead(void* data);
void TemperatureRead(void* data);
void ComputeRelay(void* data);
typedef struct action {
  Execute funcptr;
  void * data;
  unsigned int wait_delay;
}t_action;
#define NB_ACTION (7)
t_action g_actions[NB_ACTION] = { {TemperatureStartRead,&g_temp[0],READING_WAIT_TIME},
                                  {TemperatureRead,&g_temp[0],0},
                                  {TemperatureStartRead,&g_temp[1],READING_WAIT_TIME},
                                  {TemperatureRead,&g_temp[1],0},
                                  {TemperatureStartRead,&g_temp[2],READING_WAIT_TIME},
                                  {TemperatureRead,&g_temp[2],0},
                                  {ComputeRelay,NULL,0}};




byte EnumerateSensor(t_temp *p_list,byte p_list_size)
{
  byte l_nb_sensor = 0;
  byte l_nb_sensor_found = 0;
  
  while(l_nb_sensor < p_list_size)
  {
    p_list->ds->reset_search();
    if(p_list->ds->search(p_list->addr))
    {
      if(OneWire::crc8(p_list->addr, 7) == p_list->addr[7] && p_list->addr[0] == 0x28)
      {
        p_list->state = SENSOR_FOUND;
        delay(1000);
        p_list->ds->reset();
        delay(1000);
        l_nb_sensor_found++;
      }
    }
    p_list++;
    l_nb_sensor++;
  }
  
  return l_nb_sensor_found;
}

void TemperatureStartRead(void* data)
{
  t_temp* l_temp = (t_temp*)data;

  if(l_temp->state == SENSOR_FOUND)
  {
    l_temp->ds->reset();
    l_temp->ds->select(l_temp->addr);
    
    /* Lance une prise de mesure de température et attend la fin de la mesure */
    l_temp->ds->write(0x44, 1);
  }
}

void TemperatureRead(void* data)
{
  t_temp* l_temp = (t_temp*)data;
  byte ds_data[9];
  if(l_temp->state == SENSOR_FOUND)
  {
    l_temp->ds->reset();
    l_temp->ds->select(l_temp->addr);
    l_temp->ds->write(0xBE);
   
   /* Lecture du scratchpad */
    for (byte i = 0; i < 9; i++) {
      ds_data[i] = l_temp->ds->read();
    }
     
    /* Calcul de la température en degré Celsius */
    l_temp->temperature = (int16_t) ((ds_data[1] << 8) | ds_data[0]) * 0.0625; 
    Serial.print(F("Temperature : "));
    Serial.print(l_temp->temperature, 2);
    //Serial.write(176); // Caractère degré
    Serial.write('C');
    Serial.println();
  }
}

void IncOrder(void* data)
{
  t_temp* l_temp = (t_temp*)data;
  l_temp->order += 0.2;
}
void DecOrder(void* data)
{
  t_temp* l_temp = (t_temp*)data;
  l_temp->order -= 0.2;
}
void IncHysteresis(void* data)
{
  t_temp* l_temp = (t_temp*)data;
  if(l_temp->hysteresis < 2.0)
    l_temp->hysteresis += 0.1;
  
}
void DecHysteresis(void* data)
{
  t_temp* l_temp = (t_temp*)data;
  if(l_temp->hysteresis > 0.1)
    l_temp->hysteresis -= 0.1;
}

void ComputeRelay(void* data)
{
  
}

 


byte readButton()
{ 

  static byte l_button_ok = 0;
  static byte l_button_back = 0;
  static byte l_button_plus = 0;
  static byte l_button_minus = 0;

  byte l_value_button_ok = digitalRead(PIN_BUTTON_OK);
  byte l_value_button_back = digitalRead(PIN_BUTTON_BACK);
  byte l_value_button_plus = digitalRead(PIN_BUTTON_PLUS);
  byte l_value_button_minus = digitalRead(PIN_BUTTON_MINUS);
  byte l_return = 0;
  if(HIGH == l_value_button_ok && LOW == l_button_ok)
  {
    l_return = PIN_BUTTON_OK;
  }
  else if(HIGH == l_value_button_back && LOW == l_button_back)
  {
    l_return = PIN_BUTTON_BACK;
  }
  else if(HIGH == l_value_button_plus && LOW == l_button_plus)
  {
    l_return = PIN_BUTTON_PLUS;
  }
  else if(HIGH == l_value_button_minus && LOW == l_button_minus)
  {
    l_return = PIN_BUTTON_MINUS;
  } 
  
  
  l_button_ok = l_value_button_ok;
  l_button_back = l_value_button_back;
  l_button_plus = l_value_button_plus;
  l_button_minus = l_value_button_minus;

  Serial.print(F("button : "));
  Serial.print(l_value_button_ok);
  Serial.print(F(" "));
  Serial.print(l_value_button_back);
  Serial.print(F(" "));
  Serial.print(l_value_button_plus);
  Serial.print(F(" "));
  Serial.print(l_value_button_minus);
  Serial.print(F(" "));
  Serial.println();

  return l_return;
}


void DisplayTemperature(t_temp *p_sensor)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Temperature "));
  lcd.print(p_sensor->name);
  if(p_sensor->state == SENSOR_FOUND)
  {
    lcd.setCursor(5,1);
    lcd.print(p_sensor->temperature, 2);
    lcd.write('C');/* charater degré */
  }
  else
  {
    lcd.setCursor(3,1);
    lcd.print(F("no sensor"));
  }
}

void DisplayFull()
{
  lcd.clear();
  for( int l_index=0 ; l_index < NB_SENSOR;l_index++)
  {
    lcd.setCursor(6*l_index,0);
    lcd.print(g_temp[l_index].name);

    if(g_temp[l_index].state == SENSOR_FOUND)
    {
      lcd.setCursor(6*l_index,1);
      lcd.print(g_temp[l_index].temperature, 1);  
    }
    else
    {
      lcd.setCursor(6*l_index,1);
      lcd.print(F("--"));
    }
  }
  
}

void DisplaySensor1()
{
  DisplayTemperature(&g_temp[0]);
}
void DisplaySensor2()
{
  DisplayTemperature(&g_temp[1]);
}
void DisplaySensor3()
{
  DisplayTemperature(&g_temp[2]);
}

void DisplaySensorSettings(t_temp * p_sensor)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Consigne "));
  lcd.print(p_sensor->name);
  lcd.setCursor(5,1);
  lcd.print(p_sensor->order, 1);
  lcd.write('C');/* charater degré */
}

void DisplaySensor1Settings()
{
  DisplaySensorSettings(&g_temp[0]);
}
void DisplaySensor2Settings()
{
  DisplaySensorSettings(&g_temp[1]);
}
void DisplaySensor3Settings()
{
  DisplaySensorSettings(&g_temp[2]);
}
void DisplaySensorHysteresis(t_temp * p_sensor)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Hysteresis "));
  lcd.print(p_sensor->name);
  lcd.setCursor(5,1);
  lcd.print(p_sensor->hysteresis, 1);
  lcd.write('C');/* charater degré */
}

void DisplaySensor1Hysteresis()
{
  DisplaySensorHysteresis(&g_temp[0]);
}
void DisplaySensor2Hysteresis()
{
  DisplaySensorHysteresis(&g_temp[1]);
}
void DisplaySensor3Hysteresis()
{
  DisplaySensorHysteresis(&g_temp[2]);
}

enum relay_state {
  
  OFF,
  ON,
};

void DisplayRelay()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Relais"));
  //if(g_relay_status == OFF)
  {
    lcd.setCursor(7,1);
    lcd.print(F("OFF"));
  }
  /*else if (g_remay_status == ON
  {
    lcd.setCursor(7,1);
    lcd.print(F("ON"));
  }*/
}

void DisplayOff()
{
 lcd.clear();
 lcd.noDisplay();
 lcd.noBacklight();
}


typedef void (*Display)(void);

typedef struct display_menu t_display_menu;
struct display_menu {
  Display display_func;
  struct display_menu* display_ok;
  t_action action_ok;
  struct display_menu* display_back;
  t_action action_back;
  struct display_menu* display_next;
  t_action action_next;
  struct display_menu* display_prev;
  t_action action_prev;
};

t_display_menu gDisplayFull = {DisplayFull,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0}};
t_display_menu gDisplaySensor1 = {DisplaySensor1,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0}};
t_display_menu gDisplaySensor2 = {DisplaySensor2,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0}};
t_display_menu gDisplaySensor3 = {DisplaySensor3,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0}};
t_display_menu gDisplayRelay = {DisplayRelay,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0}};
t_display_menu gDisplayOff = {DisplayOff,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0}};

t_display_menu gDisplaySensor1Settings = {DisplaySensor1Settings,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{IncOrder,&g_temp[0],0},
                                NULL,{DecOrder,&g_temp[0],0}};

t_display_menu gDisplaySensor2Settings = {DisplaySensor2Settings,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{IncOrder,&g_temp[1],0},
                                NULL,{DecOrder,&g_temp[1],0}};

t_display_menu gDisplaySensor3Settings = {DisplaySensor3Settings,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{IncOrder,&g_temp[2],0},
                                NULL,{DecOrder,&g_temp[2],0}};

t_display_menu gDisplaySensor1Hysteresis = {DisplaySensor1Hysteresis,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{IncHysteresis,&g_temp[0],0},
                                NULL,{DecHysteresis,&g_temp[0],0}};

t_display_menu gDisplaySensor2Hysteresis = {DisplaySensor2Hysteresis,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{IncHysteresis,&g_temp[1],0},
                                NULL,{DecHysteresis,&g_temp[1],0}};

t_display_menu gDisplaySensor3Hysteresis = {DisplaySensor3Hysteresis,
                                NULL,{NULL,NULL,0},
                                NULL,{NULL,NULL,0},
                                NULL,{IncHysteresis,&g_temp[2],0},
                                NULL,{DecHysteresis,&g_temp[2],0}};


t_display_menu* gCurrentDisplay=&gDisplayFull;

void AddDisplayMenuNext(struct display_menu* p_head,struct display_menu* p_item)
{
  if(NULL == p_head)
  {
    return ;
  }
  if(NULL == p_item)
  {
    p_head->display_next = p_head;
    p_head->display_prev = p_head;
    return ;
  }
  p_head->display_prev->display_next = p_item;
  p_item->display_prev = p_head->display_prev;
  p_item->display_next = p_head;
  p_head->display_prev = p_item;
  return ;
}

void AddDisplayMenuOK(struct display_menu* p_head,struct display_menu* p_item)
{
  if(NULL == p_head || p_item == NULL)
  {
    return ;
  }
  struct display_menu* l_temp_menu = p_head;
  while(l_temp_menu->display_ok != NULL)
    l_temp_menu = l_temp_menu->display_ok;

  l_temp_menu->display_ok = p_item;
  p_item->display_back = l_temp_menu;
  
  return ;
}

void RefreshScreen()
{
  gCurrentDisplay->display_func();
  if(gCurrentDisplay->display_ok != NULL)
  {
    lcd.setCursor(14,1);
    lcd.print(F("->"));
  }
  if(gCurrentDisplay->display_back != NULL)
  {
    lcd.setCursor(0,1);
    lcd.print(F("<-"));
  }
}
void DisplayChange(byte l_button)
{
    lcd.display();
    lcd.backlight();

  switch(l_button)
  {
    case PIN_BUTTON_OK:
    {
      if(NULL != gCurrentDisplay->display_ok)
        gCurrentDisplay = gCurrentDisplay->display_ok;
      if(NULL != gCurrentDisplay->action_ok.funcptr)
        gCurrentDisplay->action_ok.funcptr(gCurrentDisplay->action_ok.data);
    }
    break;
    case PIN_BUTTON_BACK:
    {
      if(NULL != gCurrentDisplay->display_back)
        gCurrentDisplay = gCurrentDisplay->display_back;
      if(NULL != gCurrentDisplay->action_back.funcptr)
        gCurrentDisplay->action_back.funcptr(gCurrentDisplay->action_back.data);
    }
    break;
    case PIN_BUTTON_PLUS:
    {
      if(NULL != gCurrentDisplay->display_next)
        gCurrentDisplay = gCurrentDisplay->display_next;
      if(NULL != gCurrentDisplay->action_next.funcptr)
        gCurrentDisplay->action_next.funcptr(gCurrentDisplay->action_next.data);
    }
    break;
    case PIN_BUTTON_MINUS:
    {
      if(NULL != gCurrentDisplay->display_prev)
        gCurrentDisplay = gCurrentDisplay->display_prev;
      if(NULL != gCurrentDisplay->action_prev.funcptr)
        gCurrentDisplay->action_prev.funcptr(gCurrentDisplay->action_prev.data);
    }
    break;
      
  }
  
  RefreshScreen();
    
}

void CollectData()
{
  static byte l_current_action = 0;
  static unsigned int l_timer = 0;
  
  if(0 == l_timer)
  {
    l_timer = (g_actions[l_current_action].wait_delay==0?1:g_actions[l_current_action].wait_delay);
    g_actions[l_current_action].funcptr(g_actions[l_current_action].data);
    l_current_action++;
    if(l_current_action == NB_ACTION)
    {
      l_current_action = 0;
      RefreshScreen();
    }
  }
  /*Serial.print(F("action : "));
  Serial.print(l_current_action);
  Serial.println();*/
  l_timer--;
}


/** Fonction setup() **/
void setup() {

  /* Initialisation du port série */
  Serial.begin(115200);

  pinMode(PIN_BUTTON_OK,INPUT_PULLUP);
  pinMode(PIN_BUTTON_BACK,INPUT_PULLUP);
  pinMode(PIN_BUTTON_PLUS,INPUT_PULLUP);
  pinMode(PIN_BUTTON_MINUS,INPUT_PULLUP);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Boot up");

  AddDisplayMenuNext(gCurrentDisplay,NULL);
  AddDisplayMenuNext(gCurrentDisplay,&gDisplaySensor1);
  AddDisplayMenuNext(gCurrentDisplay,&gDisplaySensor2);
  AddDisplayMenuNext(gCurrentDisplay,&gDisplaySensor3);
  AddDisplayMenuNext(gCurrentDisplay,&gDisplayRelay);
  AddDisplayMenuNext(gCurrentDisplay,&gDisplayOff);

  AddDisplayMenuOK(&gDisplaySensor1,&gDisplaySensor1Settings);
  AddDisplayMenuOK(&gDisplaySensor1Settings,&gDisplaySensor1Hysteresis);

  //AddDisplayMenuOK(&gDisplaySensor2,&gDisplayStartTime);
  //AddDisplayMenuOK(&gDisplayStartTime,&gDisplayStopTime);


  delay(1000);
  lcd.setCursor(0,0);
  lcd.print("Search Sensor");
  g_nb_sensor = EnumerateSensor(g_temp,NB_SENSOR);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(g_nb_sensor, 1);
  lcd.setCursor(3,0);
  lcd.print("sensor found");
  
  delay(1000);
  lcd.clear();
  
}
/** Fonction loop() **/
void loop() {

  byte l_button = readButton();
  if(l_button > 0)
  {
    DisplayChange(l_button);
  }

  CollectData();
  
  delay(100);  
}
