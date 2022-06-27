////////// ручное управление //////////
// скорость 1
if (mode == 1) {
  digitalWrite(rly2, HIGH);
  delay(300);
  digitalWrite(rly1, LOW);
}
// скорость 2
else if (mode == 2) {
  digitalWrite(rly1, HIGH);
  delay(300);
  digitalWrite(rly2, LOW);
}
// выкл
else if (mode == 0) {
  digitalWrite(rly1, HIGH);
  digitalWrite(rly2, HIGH);
}
////////// /ручное управление //////////


//////////  чтение из порта //////////
if (Serial.available() > 0) {
  int str = Serial.read();
  //        if (str != 0 and str != 10 and str != 225) { // если прилетело 10 (возврат каретки) то не показываем
  //              Serial.print("\nПрилетело в порт: ");
  //              Serial.println(str);
  //          }

  if (str == 63) {  // знак ?
    Serial.print("\n\nстатистика сенсора: ");
    Serial.print(sensorStat);
    Serial.print(", длина стат. массива: ");
    Serial.print(arrStatLengthEff);
    Serial.print("\nстатистика из EEPROM: ");
    Serial.print(EEPROM.get(0, sensorStat));
    Serial.print("\nsensorThres: ");
    Serial.print(sensorThres);
    Serial.print(", sensorOverThres: ");
    Serial.print(sensorOverThres);
    Serial.print(", sensorThresHysteresys: ");
    Serial.println(sensorThresHysteresys);
    Serial.print("\n");
    delay(2000);
  }

  if (str == 43) {  // знак +
    switchModes();
  }

  if (str == 48) {  // ноль 0
    EEPROM.put(0, 0);
    memset(arrStat, 0, sizeof(arrStat));
    arrStatIndex = 0;
    arrStatLengthEff = 0;
    Serial.println("\nПоказания статистики установлены в 0");
    delay(2000);
  }
}  //////////  /serial available //////////



//////////  управление дыханием только в режиме авто //////////
if (mode == 3) {
  unsigned long currentMillisBreath = millis();
  if (speedd == 1) {
    breathInterval = 20;
  } else if (speedd == 2) {
    breathInterval = 10;
  } else {
    breathInterval = 30;
  }

  if (currentMillisBreath - prevMillisBreath >= breathInterval) {
    prevMillisBreath = currentMillisBreath;
    analogWrite(green, brightness);

    if (fadeCountTemp >= fadeCount and forward) {
      fadeCount++;
    }

    if (fadeCountTemp >= 0 and !forward) {
      fadeCount--;
    }

    // компенсация нелинейности свечения от напряжения
    if (fadeCountTemp >= 80) {
      forward = false;
    } else if (fadeCountTemp <= 10) {
      forward = true;
    }
    brightness = fadeCount + fadeCount / 1.5;  // коэф компенсации

    fadeCountTemp = fadeCount;
  }
} else if (mode == 0) {
  // standby
  unsigned long currentMillis = millis();
  if (currentMillis - prevStandbyMillis >= 700) {  // интервал моргания в standby
    prevStandbyMillis = currentMillis;
    analogWrite(yellow, standbyBrightness);
    if (standbyBrightness == 50) {
      standbyBrightness = 8;
      analogWrite(yellow, standbyBrightness);
    } else {
      standbyBrightness = 50;
      analogWrite(yellow, standbyBrightness);
    }
  }
} else {
  analogWrite(green, 0);
}

// кнопка MODE
// при нажатии кнопки на выводе сенсора около 10% от обычных показаний
// отпустили кнопку — смена MODE
if (!buttonFlag && liveSensor < sensorStat / 5) {  // обработчик нажатия
  buttonFlag = true;
  //    Serial.print("нажал ");Serial.println(liveSensor);
} else if (buttonFlag && liveSensor > sensorStat / 2) {  // обработчик отпускания
  buttonFlag = false;
  //    Serial.println("отпустил");
  switchModes();
}
}  // /loop из главного скрипта

/***************************/

//////////  переключение режимов //////////
int switchModes() {
  mode++;
  if (mode > 3) {
    mode = 0;
  }
  if (mode == 0) {
    digitalWrite(red, LOW);
    digitalWrite(green, LOW);
    digitalWrite(yellow, LOW);
    Serial.print("\nMODE: standby\n");
  }
  if (mode == 1) {
    Serial.print("\nMODE: manual speed 1\n");
    digitalWrite(yellow, HIGH);
    digitalWrite(green, LOW);
    digitalWrite(red, LOW);
  }
  if (mode == 2) {
    Serial.print("\nMODE: manual speed 2\n");
    digitalWrite(red, HIGH);
    digitalWrite(green, LOW);
    digitalWrite(yellow, LOW);
  }
  if (mode == 3) {
    // запишем статистику в епром и сбросим массив
    EEPROM.put(0, sensorStat);
    //Serial.print("sizeof(arrStat) = ");Serial.println(sizeof(arrStat));
    memset(arrStat, 0, arrLength);
    //    arrStatLengthEff = 0;

    //Serial.print("sizeof(arrStat) = ");Serial.println(sizeof(arrStat));
    //delete arrStat;
    //arrStat[arrLength] = {};
    //Serial.print("sizeof(arrStat) = ");Serial.println(sizeof(arrStat));


    digitalWrite(rly1, HIGH);
    digitalWrite(rly2, HIGH);

    digitalWrite(red, LOW);
    digitalWrite(yellow, LOW);
    Serial.print("\nMODE: reset & auto");
    Serial.print("\tстатистику в EEPROM: ");
    Serial.println(EEPROM.get(0, sensorStat));
    Serial.println();
  }
}


// количество свободного ОЗУ (RAM)
// Переменные, создаваемые процессом сборки, когда компилируется скетч
extern int __bss_end;
extern void *__brkval;
int memoryFree() {
  int freeValue;
  if ((int)__brkval == 0)
    freeValue = ((int)&freeValue) - ((int)&__bss_end);
  else
    freeValue = ((int)&freeValue) - ((int)__brkval);
  return freeValue;
}

// плавный разгон
int smoothStart() {
  digitalWrite(rly2, HIGH);  // выкл 2 скорость
  delay(300);
  digitalWrite(rly1, LOW);  // вкл 1 сорость и разгоняемся

  Serial.print("\nразгон");
  delay(1000);
  Serial.print(".");
  delay(800);
  Serial.print(".");
  delay(700);
  Serial.print(".");
  delay(600);
  Serial.print(".");
  delay(500);
  Serial.print(".");
  delay(500);
  Serial.print(".");
  delay(400);
  Serial.print(".");
  delay(400);
  Serial.print(".");
  delay(300);
  Serial.print(".");
  delay(300);
  Serial.print(".");
  delay(300);
  Serial.print(".");
  delay(300);
  Serial.print(".");
  delay(300);
  Serial.print(".");
  delay(300);
  Serial.print(".");
  delay(300);
  Serial.print(".");
  delay(300);
  Serial.print(".");
  delay(300);
  Serial.print(".");
  delay(300);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);
  Serial.print(".");
  delay(100);

  ventLow = 0;
  ventHi = 1;
}