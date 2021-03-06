# smokeTimer
Датчик "накурено" для автоматического вкл-выкл вытяжной вентиляции

Я работаю и курю дома в кабинете и у меня на лоджии установлена мощная вытяжная вентиляция. Она сильно шумит, хорошо ест электричество, и мне стало лень включать-выключать ее вручную.



**Что использовано:**
1. Arduino Nano
2. Вентилятор Вентс ТТ ПРО 200 односкоростной. Он способен сдуть самого себя со стола, но его стремные подшипники оказались огромной проблемой. Используйте любой другой однофазный.
3. Датчик MQ-2. 
4. Кнопка без фиксации какая-то китайская для переключения режимов вручную. Подключается параллельно сенсору. Я сделал так для экономии связывающих проводов, но потом использовал патч-корд, где проводов предостаточно, и из лени не стал развязывать кнопку и сенсор. Вы можете сделать иначе.
5. Светодиоды классические (6мм?) красный, желтый и зеленый. Резисторы 1к к каждому.
6. 2-метровый патч-корд с RJ-45.
7. Плата с двумя твердотельными реле (SSR). https://arduino-kit.ru/product/modul-tverdotelnogo-rele-2-kanala-5-v Твердотельники использовать необязательно, обычные тоже сгодятся.
8. Блок питания https://www.xn--80ai9an.xn--p1ai/product/1978. Китайский самый слабый, поэтому я и использовал SSR. Помещен в корпус старого трансформаторного питальника от не помню чего. Возможно от старинного мобильника типа Nokia 3310. Трансформатор оказался мертв и выброшен.



**Конструктив:**

Дуинка, питальник и плата реле собраны в корпусе питальника Nokia и все это включено в розетку под столом. Датчик, светодиоды и кнопка вынесены через патч-корд и розетки RJ-45 в отдельный корпус, висящий над монитором. В него я курю, датчик нюхает дым и подает сигнал на ардуину.



**Логика работы:**
 
 0. Памяти в дуине мало, поэтому никакого ООП.
 1. Запускаем когда в комнате не накурено.
 2. Курим.
Пока дыма нет — вентилятор откл и пишется статистика в массив arrStat. Когда дыма больше чем среднее из arrStat на sensorThres — вентилятор включатеся на скорость 1 и ждет. Это может быть и при видимом отсутствии дыма, но датчику виднее, так что пусть проветривает
Когда дыма больше чем среднее из arrStat на sensorOverThres, включаем скорость 2.
Статистика на скорости 2 не пишется кроме одного случая: если мы дуем-дуем 300 секунд, а показатели всё не падают. Это значит что подоспело суточное колебание чистоты воздуха и надо включить запись статистики чтобы скорректировать ее на новые реалии. Такое колебание довольно существенное и сильно влияет на показания датчика.
Показания датчика падают и вент переключается на скорость 1 пока разница diff не станет 0.



**Электросхема:**

<a href="https://ibb.co/VNT1HNv" target="_blank"><img src="https://i.ibb.co/VNT1HNv/smoke-Sensor.png" alt="smoke-Sensor" border="0" /></a>

Не показаны кнопка и светодиоды, расположите их как удобно и где удобно. Можете не использовать их вовсе. 
С1 уже есть в вашем вентиляторе, возьмите еще один конденсатор с такими же параметрами и поставьте его как С2. Можно поиграть его емкостью, можно не использовать пониженную (1) скорость вовсе и не колхозить С2 c резистором.



**Примечания:**

1. Если вы используете SSR, их нужно защитить от обратного выброса электродвигателя. Для этого параллельно рабочей обмотке висит варистор U1 вольт примерно на 500 и джоулей примерно на 100, я уже забыл сколько. Если вы используете механические реле, то варистор не нужен.
2. Пониженная (1) скорость вентилятора получена путем последовательного включения в цепь электровигателя неполярного конденсатра 3...6мкФ/400В. Подберите емкость под свой двигатель, она будет примерно равна емкости пускового конденсатора, если Вентс до сих пор ставит в эти вентиляторы асинхронные односкоростные двигатели. Скорее всего сейчас там применяются двухскоростные и вам понадобится только добавить варистор при условии использованиия твердотельных реле.
3. После переключения со 1 скорости на 2 на этом дополнительном конденсаторе остается энергия, способная убить любые ваши реле за полгода. При циклическом переключении вентилятора этот конденсатор будет разряжаться на ваше реле, и чтобы его защитить, поставьте параллельно этому кондесатору резистор примерно на 100кОм мощностью 1Вт (с запасом).

Удачи и не забывайте разряжать конденсаторы при каждом вмешательстве ;)
