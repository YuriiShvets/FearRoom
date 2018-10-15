#include <SPI.h>

//Add the SdFat Libraries
#include <SdFat.h>
#include <SdFatUtil.h>

#include <SFEMP3Shield.h>

SdFat sd;

void(* resetFunc) (void) = 0;

class Hydraulics{
     
  private:
  
    int pin;            //виход до якого підключений
    boolean state;      //стан
    int StartRead;      //Початок області пам'яті
    int EndRead;        //кінець області пам'яті
    unsigned long Time;
    int index;          //індекс розпізнаного масиву
    int scriptSize;
    
  public:
 
    Hydraulics(int inPin, int inStart, int inEnd){
      pin = inPin;
      StartRead = inStart;
      EndRead = inEnd;
      Time = 0;
      index = inStart - 1;                                            //думаєш чому "-1"? подивись в Update
      scriptSize = inEnd - inStart;

      state = false;
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
    }
    
    boolean Update(int *script){
      if((Time == 0 || millis() - Time > script[index]%10000) && index <= EndRead){
        Time = millis();
        index++;
      }
      if(index <= EndRead){
        if(script[index] >= 20000)
          digitalWrite(pin, HIGH);
        else
          digitalWrite(pin, LOW);
      }
      else{
        digitalWrite(pin, LOW);
        return false;
      }
      return true;
    }
    
    void Update(){
      if(state)
        digitalWrite(pin, HIGH);
      else
        digitalWrite(pin, LOW); 
    }

    void setState(boolean inState){
      state = inState;
    }

    void Reset(){
      index = StartRead - 1;
    }
};

class Hydraulics2{      //інвертована
  
  private:
  
    int pin;            //виход до якого підключений
    boolean state;      //стан
    int StartRead;      //Початок області пам'яті
    int EndRead;        //кінець області пам'яті
    unsigned long Time;
    int index;          //індекс розпізнаного масиву
    int scriptSize;
    
  public:
 
    Hydraulics2(int inPin, int inStart, int inEnd){
      pin = inPin;
      StartRead = inStart;
      EndRead = inEnd;
      Time = 0;
      index = inStart - 1;                                            //думаєш чому "-1"? подивись в Update
      scriptSize = inEnd - inStart;

      state = false;
      pinMode(pin, OUTPUT);
      digitalWrite(pin, HIGH);
    }
    
    boolean Update(int *script){
      if((Time == 0 || millis() - Time > script[index]%10000) && index <= EndRead){
        Time = millis();
        index++;
      }
      if(index <= EndRead){
        if(script[index] >= 20000)
          digitalWrite(pin, LOW);
        else
          digitalWrite(pin, HIGH);
      }
      else{
        digitalWrite(pin, HIGH);
        return false;
      }
      return true;
    }
    
    void Update(){
      if(state)
        digitalWrite(pin, LOW);
      else
        digitalWrite(pin, HIGH); 
    }

    void setState(boolean inState){
      state = inState;
    }

    void Reset(){
      index = StartRead - 1;
    }
};

class Listener{
  
  private:
  
    char data;

  public:
  
    Listener(){
     data = '0';
    }

    void Update(){
      Serial.write('0');
      if(Serial.available()){
          data = Serial.read();
      }
      else data = '0';
    }

    char getData(){
     return data;
    }
};

class Sensor{
  private:
    int pin;
    int Delay;
    int numberOfFilterIterations;
    boolean state;
    SdFile file;
    
  public:
    Sensor(int inPin){
      pin = inPin;
      pinMode(pin, INPUT);
      state = false;
      Delay = 0;
      numberOfFilterIterations = 0;
      //------------------костиль який читає відстань із флешки--------
      
      if(file.open("sensor.txt")){
        for(int i = 0; i < 4; i++){   //кількість циклів фільтрафії
          numberOfFilterIterations *= 10;
          numberOfFilterIterations += (file.read() - 48);
        }
      }
      if(file.open("delay.txt")){
        for(int i = 0; i < 4; i++){
          Delay *= 10;
          Delay += (file.read() - 48);
        }
      }
    file.close();
    
    //---------------------Кінець костиля------------------------------
    
    }

    void Update(){
      if(!state)
        state = filter();
    }

    boolean getState(){
      if(!state)
        return state;
      state = false;
      return true;
    }

    boolean filter(){
      int sum = 0;
      int buf;                                //змінна де зберігається значення сенсора
        for(int i = 0; i < numberOfFilterIterations; i++){
          if(!digitalRead(pin))
            sum++;
        }
          if(sum > (float)numberOfFilterIterations * 0.5)
            return true;
          return false;
    }

    int getDelay() {
      return Delay;
    }
};

class Sekyurotron{
  
  private:
  
    float keys [8];
    SdFile file;
    float key;
    int keyIndex;         //індекс ключа считаного із флешки
    int fileIndex;        //індекс считаний із флешки
    int timer;            //кількість спрацювань,що залишилася
    
  public:

  Sekyurotron(){
    //keys = new float [8];
    keys[0] = 63253497;
    keys[1] = 80745013;
    keys[2] = 63670818;
    keys[3] = 69881926;
    keys[4] = 47414777;
    keys[5] = 92007094;
    keys[6] = 17568987;
    keys[7] = 13758331;
    key = 0;
    //file.open("key.txt");
    if(file.open("key.txt"))
      for(int i = 0; i < 8; i++){
        key *= 10;
        //Serial.println(keys[i]);
        key += (file.read() - 48);
      //  Serial.println(key);
      }
      Serial.println(key);
    file.close();
    
    for(int i = 0; i < 8; i++)
      if(abs((int)(keys[i] - key)) < 10){
        keyIndex = i;
        break;
      }     
    //file.open("script2.txt");
    if(file.open("script2.txt")){
      fileIndex = 10;
      fileIndex = file.read();
      Serial.println(fileIndex);
      timer = 0;
      timer = file.read();
      Serial.println(timer);
    } 
    file.close();
    
    if(keyIndex > fileIndex){   
      fileIndex = keyIndex;
      sd.remove("script2.txt");
      //file.open("script2.txt", O_RDWR | O_CREAT | O_AT_END);
      timer = 250;
      if(file.open("script2.txt", O_RDWR | O_CREAT | O_AT_END)){
        file.print((char)fileIndex);
        file.print((char)timer);
      }
      file.close();
    }
    //Serial.println(keyIndex);
  }

  void decrementTimer(){
    if(timer > 0)
      timer--;
    sd.remove("script2.txt");
    //file.open("script2.txt", FILE_WRITE);
    if(file.open("script2.txt", O_RDWR | O_CREAT | O_AT_END)){
      file.print((char)fileIndex);
      file.print((char)timer);
    }
    file.close();
  }
  boolean getTimerState(){
    Serial.println(timer);
    if(timer > 0)
      return true;
      return false;
  }

  boolean getFileIndexState(){
    Serial.println(fileIndex);
    if(fileIndex == 7)
      return true;
    return false;
  }
};

class Music{
  
  private:
    SFEMP3Shield MP3player;
    
  public:
   Music(){
      MP3player.begin();
   }
   
   void Update(){
     union twobyte mp3_vol;
     MP3player.playTrack(1);
     mp3_vol.word = MP3player.getVolume();
     mp3_vol.byte[1] = 1;
     MP3player.setVolume(mp3_vol.byte[1], mp3_vol.byte[1]);
   }

   void error() {
     union twobyte mp3_vol;
     MP3player.playTrack(0);
     mp3_vol.word = MP3player.getVolume();
     mp3_vol.byte[1] = 1;
     MP3player.setVolume(mp3_vol.byte[1], mp3_vol.byte[1]);
   }
   
   void StopMusic(){
     MP3player.stopTrack();
   }
};

class Program{
  
  private:

    Sekyurotron *sekyurotron;
    Sensor *button;
    //Listener *listener;
    Hydraulics *hydr1;
    Hydraulics *hydr2;
    Hydraulics *lamp;
    Music *sound;
    int dataSize;
    int script[192];
    int scriptSize;
    
 public:
 
    Program(){
      hydr1 = new Hydraulics(10, 0, 64);
      hydr2 = new Hydraulics(5, 64, 128);
      lamp = new Hydraulics(3, 128, 192);
      button = new Sensor(A3);

      
      sekyurotron = new Sekyurotron();


      
      dataSize = 1536;
      scriptSize = 192;
      //script = new int [scriptSize];
      SdFile ScriptFile;
      //ScriptFile.open("script.txt");
      if(ScriptFile.open("script.txt")){
      
        digitalWrite(A1,HIGH);
    
        for(int i = 0; i < scriptSize; i++)
          script[i] = 0;
        for(int i = 0, j = -1; i < dataSize; i++){
          char data = ScriptFile.read();
          if(data == '<'){
            if(script[j] == 20000)                  //заміна включення з нульовим часом на виключення (костиль ібо не встигаю)
              script[j] = 10000;
            j++;
          }
          if(data == '1' || data == '2' || data == '3' || data == '4' || data == '5' || data == '6' || data == '7' || data == '8' || data == '9' || data == '0'){
            script[j] *= 10;
            script[j] += (data - 48);
          }
        }
      }
      ScriptFile.close();
      //listener = new Listener();
      sound = new Music();
  }

    void Update(){
      button -> Update();
      if(button -> getState() && (sekyurotron -> getFileIndexState() || sekyurotron -> getTimerState())){
        delay(button->getDelay());  //затримка
        int sum;
        sound -> Update();
        do{
           sum = 0; 
           sum = (int)(hydr2 -> Update(script)) + (int)(hydr1 -> Update(script)) + (int)(lamp -> Update(script));
        }
        while(sum > 0);
        sound -> StopMusic();
        hydr1 -> Reset();
        hydr2 -> Reset();
        lamp -> Reset();
        sekyurotron ->decrementTimer();
        delay(20000); // задержка от следующего включения
        resetFunc();
      }
      else {
         if(!sekyurotron -> getFileIndexState()) 
         {


          sound -> error();

          
          for (int i=0; i<100; i++);
          {
          digitalWrite(A1,HIGH);
          delay(200);
          digitalWrite(A1,LOW);
          delay(200);
          }
         }
         
         }
      /*
        if(button -> getState() && !sekyurotron -> getFileIndexState() && !sekyurotron -> getTimerState()) {
          sound -> error();
          //мигание светодиода
          for (int i=0; i<100; i++);
          {
          digitalWrite(A1,HIGH);
          delay(100);
          digitalWrite(A1,LOW);
          delay(100);
          }
        }
        */
      }
    
};

class Programer{
  
  private:
    Listener *listener;
    Hydraulics *hydr1;
    Hydraulics *hydr2;
    Hydraulics *lamp;
  
  public:
  Programer(){
    hydr1 = new Hydraulics(10, 0, 64);
    hydr2 = new Hydraulics(5, 64, 128);
    lamp = new Hydraulics(3, 128, 192);
    //hydr1 -> setState(true);
    //hydr2 -> setState(true);
    //lamp -> setState();
    listener = new Listener();
  }
  
  void Update(){
    listener -> Update();
      switch(listener -> getData()){
        case '1':{
          hydr1 -> setState(false);
          //Serial.println("hydr1 - false");
          break;
        }
        case '2':{
          hydr1 -> setState(true);
          //Serial.println("hydr1 - true");
          break;
        }
        case '3':{
          hydr2 -> setState(false);
          //Serial.println("hydr2 - false");
          break;
        }
        case '4':{
          hydr2 -> setState(true);
          //Serial.println("hydr2 - true");
          break;         
        }
        case '5':{
          lamp -> setState(false);
          //Serial.println("hydr2 - false");
          break;
        }
        case '6':{
          lamp -> setState(true);
          //Serial.println("hydr2 - true");
          break;         
        }
      }
      hydr1 -> Update();
      hydr2 -> Update();
      lamp -> Update();
    }
};    

void setup() {
  pinMode(10, OUTPUT);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);      // красный диод
  
  digitalWrite(A0, HIGH); // проверка работоспособновсти чипа
//digitalWrite(A1, HIGH); 
  
 // digitalWrite(10, HIGH);
  
  Serial.begin(9600);
  Serial.println("Power is ON");
  
  if (!sd.begin(9, SPI_HALF_SPEED)){

    Programer *programer = new Programer();
    while(true) 
      programer -> Update();

 // digitalWrite(A1, LOW);
 //   digitalWrite(A0, LOW);
    //  digitalWrite(A4, HIGH); 
    //resetFunc();
  }
  Program *one = new Program();
  while(true){
    one -> Update();
  }
}

void loop() {
  
}
