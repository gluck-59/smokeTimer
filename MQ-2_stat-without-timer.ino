/* использование
 1. запускаем когда в комнате не накурено
 2. курим

    пока дыма нет — вентилятор откл и пишется статистика в массив arrStat
    когда дыма больше чем среднее из arrStat на sensorThres — вентилятор включатеся на скорость 1 пока дыма не станет 0
    это может быть и при видимом отсутствии дыма, но датчику виднее, так что пусть проветривает

    когда дыма больше чем среднее из arrStat на sensorOverThres, включаем скорость 2 
    статистика на скорости 2 не пишется кроме одного случая: если мы дуем-дуем 300 секунд, а показатели всё не падают
    это значит что подоспело суточное колебание чистоты воздуха и надо включить запись статистики чтобы скорректировать ее на новые реалии
    такое колебание довольно существенное и сильно влияет на показания датчика 
*/


/* отладка */
// отправлять в терминал порта:
// плюс (+): вент выкл/скорость1/скорость2/авто
// знаковопроса (?): показать пороги, статистику и что хранится в памяти

//////////////////////////////////////////////////////////////////////////

#include <EEPROM.h>

#define smokePin A7 // датчик типа  MQ-2
#define red 7       // красный светодиод
#define green 9     // зеленый светодиод
#define yellow 10   // желтый светодиод
#define rly1 11     // реле 1
#define rly2 12      // реле 2 (ШИМ c этим мотором не используется)

byte mode = 3;                  // режим работы
int liveSensor;                 // показания датчика дыма
int diff = 0;                   // разница между датчиком и статистикой

byte sensorThres = 10;          // разница от статистики "немного накурено"
byte sensorOverThres = 35;      // разница от статистики "сильно накурено"
byte sensorThresHysteresys = 5; // гистерезис для выключения чуть позже чем уровень включения

int ventHiTimeGlobal = 10;      
int ventHiTime = ventHiTimeGlobal;
bool hiTimerTick;

byte ventLow = 0;  // скорость 1
byte ventHi = 0;   // скорость 2
byte prevVentLow = ventLow;
byte prevVentHi = ventHi;

byte speedd = 0;  // для вывода
byte subProg = -1;// для вывода (отобразится 255)

byte pastRly1; // для выдержки при переключениях реле
byte pastRly2; // для выдержки при переключениях реле

unsigned long prevMillis = 0;             // общее время
unsigned long prevStandbyMillis = millis(); // для standby
unsigned long prevMillisBreath = 0;       // для дыхания
byte  breathInterval = 30;                // частота дыхания
byte  brightness = 8;                     // минимальная яркость дыхания
int fadeCount = 0;
int fadeCountTemp = fadeCount;
bool forward = true;
byte standbyBrightness = 50;               // яркость желтого светодиода в режиме ВЫКЛ
bool buttonFlag = false;


// для инкремента/декремента времени работы
int ventTimeInterval = 3000;              // частота оценки задымленности
int printInterval = ventTimeInterval;     // частота вывода в порт
unsigned long prevMillisVent = 0;

// для статистики
const int arrLength = 250; // можно увеличить если памяти достаточно
int arrStat[arrLength] = {};
int arrStatIndex = 0;
int arrStatLengthEff = 0;
long arrStatSum;

unsigned long prevMillisStat = 0;
int statTimeInterval = 1000; // периодичность записи стат.массива
int sensorStat; // статистическое значение доставаемое из массива


void setup() {
  Serial.begin(19200);
  pinMode(rly1, OUTPUT);
  pinMode(rly2, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(yellow, OUTPUT);

  digitalWrite(rly1, HIGH); // Выключаем реле
  digitalWrite(rly2, HIGH); // Выключаем реле
  pastRly1 = rly1;
  pastRly2 = rly2;

  analogWrite(green, brightness);
  digitalWrite(red, LOW);
  digitalWrite(yellow, LOW);

  sensorStat = EEPROM.get(0, sensorStat);
  
  Serial.print("\n\n\n\n\n\n================================================================");
  Serial.print("\n\nВент ВКЛ = logic 0, вент ВЫКЛ = logic 1\n\n");
  Serial.print("-= ДЕБАГ =-:\n");
  Serial.print("отправить в терминал порта:\n");
  Serial.println("плюс (+) режимы — вент выкл/скорость1/скорость2/автомат");
  Serial.println("вопрос (?) — показать статистику и тресхолды");
  Serial.println("ноль (0) — сброс показаний статистики");
  Serial.println();
  Serial.print("Статистика из EEPROM: "); Serial.println(sensorStat);
  Serial.print("Свободно памяти: "); Serial.println(memoryFree());
  Serial.println("Включать когда не накурено");Serial.println();
  delay(2000);
}

void loop() {
  liveSensor = analogRead(smokePin);

  /////// блок статистики /////////
  unsigned long currentMillisStat = millis();
if (currentMillisStat - prevMillisStat >= ventTimeInterval) 
  {
    prevMillisStat = currentMillisStat;

    // условия записи:
    if (mode == 0 or mode == 3) {
      // сброс на начало массива при достижении последнего индекса
      if (arrStatIndex >= arrLength) {
        arrStatIndex = 0;
      }
      
      // запись
      if (speedd < 2) {
        arrStat[arrStatIndex] = liveSensor;
        arrStatIndex++;
      } 
 
      // особый режим: если дуем-дуем а показания особо не падают, разрешим писать в статистику чтобы поднять ее до новой реальности
      else if (speedd == 2 and ventHiTime > 300 /*and diff > sensorOverThres * 1.5*/)  {
        arrStat[arrStatIndex] = liveSensor;
        arrStatIndex++;
      }
    }
  }
  /////// /блок статистики /////////

  ////////// блок условий //////////
  unsigned long currentMillisVent = millis();
if (mode == 3 and currentMillisVent - prevMillisVent >= statTimeInterval) 
  {
      prevMillisVent = currentMillisVent;
    
    // чтение статистики
    for(int i = 0; i < sizeof(arrStat)/sizeof(int); i++)  {
      if (arrStat[i] > 0) 
      {
        arrStatSum += arrStat[i];
        arrStatLengthEff++;
      }
    }
    sensorStat = arrStatSum / arrStatLengthEff;
  
    // watchdog на полное отсутсвие статы
    if (sensorStat <= 0) {
//    Serial.println("if sensorStat <= 0");
      sensorStat = EEPROM.get(0, sensorStat);
    }
    diff = liveSensor - sensorStat; 
    ////////// /блок условий //////////

    
    /////////// логика режимов ////////////
    // 0 не накурено
      if(diff < 0 /*and speedd > 0*/) {
        subProg = 0;
        ventHi = 0;
        ventLow = 0;
        ventTimeInterval = 3000;
      } 
      else if (!hiTimerTick and speedd != 2 and ventHiTime != ventHiTimeGlobal) {
        subProg = 2;
        ventHiTime = ventHiTimeGlobal;
      } 
      
    
     // 1 слегка накурено
      if(
        (speedd == 0 and diff > sensorThres and diff <= sensorOverThres) or 
        (speedd == 2 and diff <= sensorOverThres - sensorThresHysteresys)
        ) {
        subProg = 3;
        ventHi = 0;
        ventLow = 1;
        ventTimeInterval = 1000;
      } 
      
    
     // 2 сильно накурено    
      if (diff > sensorOverThres) {
        subProg = 4;
        
        // плавный разгон        
        if (speedd < 2) { 
          smoothStart();
        }
        
        ventHi = 1;
        ventLow = 0;
        ventHiTime++;

        if(diff > sensorOverThres*2) {
          digitalWrite(red, HIGH);
          delay(10);
          digitalWrite(red, LOW);
        }
      } 
    } //venttimeInterval
    ////////// /логика режимов ///////////


    ////////// вывод в реле //////////
    // логический 0 ***** ВКЛЮЧАЕТ ****** реле
    /* delay — защита от случайного вкл оба реле вместе
    *  в этом случае конденсатор первой скорости разрядится на реле */
    if (mode == 3) { // вывод только в режиме "автомат" (mode = 3)
      unsigned long currentMillis = millis();
  
      // стоп
      if (ventLow == 0 and ventHi == 0 and (prevVentLow != ventLow or prevVentHi != ventHi)) {
        Serial.print("\n—>переключаем на СТОП ");
        // в реле 0,0
        digitalWrite(rly1, HIGH); // откл
        digitalWrite(rly2, HIGH); // откл
        speedd = 0;
        ventHiTime = ventHiTimeGlobal;
      }
  
      // скорость 1
      else if (ventLow == 1 and (prevVentLow != ventLow or prevVentHi != ventHi)) {
        Serial.print("\n—>переключаем на скорость 1 ");
        // в реле 1,0
        digitalWrite(rly2, HIGH);
        delay(100);
        digitalWrite(rly1, LOW);
        speedd = 1;
        ventHiTime = ventHiTimeGlobal;
      }
  
      // скорость 2
      else if (ventHi == 1 and (prevVentLow != ventLow or prevVentHi != ventHi)) {
        Serial.print("\n—>переключаем на скорость 2 ");
        // в реле 0,1
        digitalWrite(rly1, HIGH); // откл
        delay(100);
        digitalWrite(rly2, LOW);  // вкл
        speedd = 2;
      }
      ////////// /вывод в реле //////////

  
      //////////  пишем в порт ////////// 
      if (currentMillis - prevMillis >= printInterval-500) {
        prevMillis = currentMillis;
  
        // еще одно чтение статистики для записи в порт (старое к этому времени уже протухло)
        arrStatSum = 0;
        arrStatLengthEff = 0;
        for(int i = 0; i < sizeof(arrStat) / sizeof(int); i++)  {
          if (arrStat[i] > 0) {
            arrStatSum += arrStat[i];
            arrStatLengthEff++;
          }
        }
        sensorStat = arrStatSum / arrStatLengthEff;
        Serial.print("\n =");Serial.print(subProg);
        Serial.print("= DIFF: ");Serial.print(diff);
        Serial.print("\tsensor: "); Serial.print(liveSensor); Serial.print(", stat: "); Serial.print(sensorStat);
        Serial.print("\tarr: ");Serial.print(arrStatIndex);Serial.print("/");Serial.print(arrStatLengthEff); 
        Serial.print(" \tspeedd: "); Serial.print(speedd); 
        Serial.print("\thiTimer: ");Serial.print(ventHiTime);
      }
      //////////  /пишем в порт ////////// 
  
      prevVentLow = ventLow;
      prevVentHi = ventHi;
    }


// ...окончание loop в functions 
