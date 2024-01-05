const int enPin=8;

//USER CODE BEGIN THERE

//USER CODE END THERE


/*
 * Класс для работы с шаговыми моторами инкапуслирует в себя механику работы и предоставляет несколько методов 
 * 
 * 1) Конструктор Motor имя(указатель на массив пинов моторов, количество моторов, указатель на массив пинов направления, номер пина концевого выключателя...
 *                          количество шагов для полного оборота, максимальную скорость мм/с, минимульную скорость мм/с, шаг резьбы);
 *    Один объект класса двигает моторы в одинаковых или разных направлениях, но только на одно расстояние и с одной скоростью, нет ограничения на использование
 *    нескольких объектов, например объект ВсеМоторы, МоторыВерхнегоЭтажа, МоторыНижнегоЭтажа. 
 *    
 * 2) go(дистанция мм, скорость мм/с, ускорение (количество шагов за которое мотор наберет скорость), флаг); 
 *    Метод задает мотору перемещение на задауню дистанцию с заданой скоростью и ускорением. При этом если нет флага forsed, мотор остановится при ударе о свой концевой выключатель. 
 *    Иногда надо подвинуть мотор при нажатом концевом выключателе - это делается указанием флага "forsed"
 *    
 * 3) SetDirection(указатель на массив пинов, количество пинов, направление) Для всех макетов 0 - вверх, 1 - низ. 
 * 
 * 4) Initilaze(); моторы двигаются вверх до концевика, находя свою позицию, и возвращаются на рабочую позицию - 200мм. 
*/
class Motor
{
  /*Класс для работы с мотором, в конструкторе указаетль на массив пинов, количество пинов, указатель на масив пинов направления, количество шагов для одного оборота
  максимальная скорость, минимальная скорость, шаг резьбы мотора*/
  public:
    Motor(uint8_t *_StepPin, uint8_t _MotorsNum, uint8_t *_DirPin,uint8_t _TopEndPin,uint8_t _BotEndPin,
          unsigned short int _StepsToRotate, unsigned int _maxSpeed, unsigned int _minSpeed, unsigned short int _ScrewStep)
    {
      //инициализация полей 
      StepPin = _StepPin;
      DirPin = _DirPin;

      TopEndPin = _TopEndPin; 
      BotEndPin = _BotEndPin;
      
      StepsToRotate = _StepsToRotate;
      maxSpeed = _maxSpeed;
      minSpeed = _minSpeed;
      ScrewStep = _ScrewStep*4;
      MotorsNum = _MotorsNum;

      //инициализация одометрии (пока не доделана)
      coordinate = 0.0; 
    };

    /*установка направления для моторов*/
    void setDirection(uint8_t *Pin, uint8_t Num, uint8_t dir)
    {
      for(size_t i = 0;i<Num;i++)
      {
        Serial.println(Pin[i]);
        Serial.println(dir);
        Serial.println();
        digitalWrite(Pin[i], dir);
      }
      
    };

    /*метод движения, принимает нужную дистанцию, скорость, ускорение и флаг forsed, если флаг установлен, игнорируются удары в концевики, будьте аккуратны!*/
    bool go(int travel, unsigned int travel_speed, int acceleration_steps, bool forsed = 0) 
    {
      if(travel_speed > this->maxSpeed) 
      {
        Serial.print("WRN: speed specified is too high, conflict with constructor method. The speed has been limited to");
        Serial.println(maxSpeed); 
        travel_speed = maxSpeed;
      }
     
      int travel_steps = (travel/ScrewStep*StepsToRotate-2*(acceleration_steps))/(2);
      int delay_min = 10125;
      int delay_travel = ScrewStep*1000000/(StepsToRotate*travel_speed) - 500;
      double delta = double((delay_min - delay_travel)/(acceleration_steps));
      
      double delta_x;
      if(direct == 1) {delta_x = (double)ScrewStep/StepsToRotate;}
      else {delta_x =  - (double)ScrewStep/StepsToRotate;}
      
      int current_delay = delay_min;
      for(size_t i = 0; i<acceleration_steps; i++)
      {
        if((digitalRead(TopEndPin)==1 || digitalRead(BotEndPin)==1)&& !forsed)
        {
          Serial.println("ERR: Unexpected limit switch pressing, engine sttoped");
          return 0; 
        }
        WriteMotors(HIGH);
        coordinate+=delta_x;
        delayMicroseconds(500);
        WriteMotors(LOW);
        delayMicroseconds(current_delay);
        //если собираетесь менять значение кэффициентов нелинейности, будьте уверенны что вы понимаете, что делаете
        // всего часов тут потрачено - 4
        /*коэффициенты нелинейности ускорения обеспечивают плавное ускорение, задаваемое от шагов, чем быстрее идут шаги мотора, тем быстрее увеличивается скорость 
        приростом на каждом шаге, процесс разбит на 4 линейных этапа функции 1/x*/
        if(i%10 == 0)
        {
          if(i<acceleration_steps/4) {
            current_delay-=delta*30; 
          }
          if(i>acceleration_steps/4 && i <= acceleration_steps/2){
            current_delay-=delta*6; 
          }
          if(i>acceleration_steps/2 && i <= acceleration_steps*3/4) {
            current_delay-=delta*2.5; 
          }
          if(i>=acceleration_steps*3/4){
          current_delay-=delta*1.2; 
        }
        if(current_delay < delay_travel)Serial.println("overload");;
        }
      }
      

      for(size_t i = 0; i < travel_steps; i++)
      {
        if((digitalRead(TopEndPin)==1 || digitalRead(BotEndPin)==1)&& !forsed)
        {
          Serial.println("ERR: Unexpected limit switch pressing, engine sttoped");
          return 0; 
        }
        WriteMotors(HIGH);
        delayMicroseconds(500);
        coordinate+=delta_x;
        WriteMotors(LOW);
        delayMicroseconds(delay_travel);

      }
      current_delay = delay_travel;
      for(size_t i = 0; i<acceleration_steps; i++)
      {
        if((digitalRead(TopEndPin)==1 || digitalRead(BotEndPin)==1)&& !forsed)
        {
          Serial.println("ERR: Unexpected limit switch pressing, engine sttoped");
          return 0; 
        }
        WriteMotors(HIGH);
        coordinate+=delta_x;
        delayMicroseconds(500);
        WriteMotors(LOW);
        delayMicroseconds(current_delay);
       
        if(i%10 == 0)
        {
          if(i<acceleration_steps/4) {
            current_delay+=delta*1.2; 
          }
          if(i>acceleration_steps/4 && i <= acceleration_steps/2){
            current_delay+=delta*2.5; 
          }
          if(i>acceleration_steps/2 && i <= acceleration_steps*3/4) {
            current_delay+=delta*6; 
          }
          if(i>=acceleration_steps*3/4){
          current_delay+=delta*30; 
          }
        }
        
      }
       return 1; //Если поток дошел до сюда и не прервался ударом о концевик, функция возвращает истину
    };

    /*процедура инициализации, стандартное направление 0 - вверх*/
    void initilaze()
    {
      setDirection(DirPin,3,0); //установка направления
      while(go(600, 10, 1600)); //плавный ход до момента удара в концевой выключатель
      setDirection(DirPin,3,1); //смена направления
      go(10,5,100,"forsed"); //отъехать назад игнорируя нажатый концевик
      setDirection(DirPin,3,0); //установка направления
      while(go(100, 4, 1600)); //плавный ход до момента удара в концевой выключатель
      setDirection(DirPin,3,1); //смена направления
      
      go(200,10,1600,"forsed"); //движение в рабочую позицию 
    }
    
  private:
    uint8_t *StepPin;
    uint8_t  MotorsNum;
    uint8_t *DirPin;
    uint8_t  TopEndPin;
    uint8_t BotEndPin;
    unsigned short int StepsToRotate;
    unsigned int maxSpeed;
    unsigned int minSpeed;
    unsigned int ScrewStep;
    double coordinate;
    uint8_t direct;

    //прохождение одного шага для всех моторов объекта
    void WriteMotors(uint8_t arg)
    {
      for(size_t i = 0; i<MotorsNum; i++)
      {
        digitalWrite(StepPin[i], arg);
      }
    }
    
};

uint8_t AllstepPin[]={2,3,4};
uint8_t AlldirPin[]={5,6,7};
uint8_t TopFloorStepPin[]= {4};
uint8_t TopFloorDirPin[] = {7};
uint8_t BottomFloorStepPin[]={2,3};
uint8_t BottomFloorDirPin[] = {5,6};
Motor All(AllstepPin,3, AlldirPin,9,10,1600, 16 ,1, 2);
Motor Top(TopFloorStepPin,1,TopFloorDirPin,9,10,1600, 16 ,1, 2);
Motor Bot(BottomFloorStepPin,2,BottomFloorDirPin,9,10,1600, 16 ,1, 2);

void setup() 
{
  Serial.begin(9600);
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, LOW);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  
 
  Serial.println("Initialize");

  //USER CODE BEGIN THERE
  //Top.initilaze();
  //Bot.initilaze();
  //USER CODE END THERE
}

void loop() 
{
  //USER CODE BEGIN THERE
      All.setDirection(AlldirPin,3,0);
      if(!All.go(170,10,1200)) 
      {
        Serial.println("error 0");
      }

      delay(1000);

      All.setDirection(AlldirPin,3,1);
      if(!All.go(170,10,1200))
      {
        Serial.println("error 1");
      }
      delay(1000);
   //USER CODE END THERE
}
