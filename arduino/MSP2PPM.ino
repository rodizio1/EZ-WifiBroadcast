//////////////////////CONFIGURATION///////////////////////////////
#define CHANNEL_NUMBER 8  //set the number of chanels
#define CHANNEL_DEFAULT_VALUE 1200  //set the default servo value
#define FRAME_LENGTH 22500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PULSE_LENGTH 300  //set the pulse length
#define onState 1  //set polarity of the pulses: 1 is positive, 0 is negative
#define sigPin 10  //set PPM signal output pin on the arduino
#define FAILSAFE 500 //activate failsafe after 1000ms of inactivity
#define FAILSAFE_THROTTLE_VALUE 1350 // value we put to throttle if failsafe is activated

/*this array holds the servo values for the ppm signal
 change theese values in your code (usually servo values move between 1000 and 2000)*/
int ppm[CHANNEL_NUMBER];
uint8_t msp_packet[22];
int msp_packet_index=0;
long int last_receive;


void setup(){  

  //initiallize default ppm values
  for(int i=0; i<CHANNEL_NUMBER; i++){
      ppm[i]= CHANNEL_DEFAULT_VALUE;
  }

  last_receive=millis();
  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, !onState);  //set the PPM signal pin to the default state (off)

  Serial.begin(115200);
  
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();

}


void loop(){

  uint8_t c;
  int crc;
  int i;
  int channel_index;
  
  while(Serial.available())
  {
    c=Serial.read();
    
    if(msp_packet_index==0 && c!=36) // $ character
    {
      continue;
    }
    
    if(msp_packet_index==1 && c!=77) // M character
    {
      continue;
    }

    if(msp_packet_index==2 && c!=60) // < character
    {
      continue;
    }

    msp_packet[msp_packet_index]=c;

    if(msp_packet_index==21) // packet complete
    {
      crc=0;
      
      for(i=3;i<21;i++)
      {
        crc^=msp_packet[i];
      }

      //msp fame valid
      if(crc==msp_packet[21])
      {
      	last_receive=millis();
        channel_index=0;
        
        for(channel_index=0;channel_index<CHANNEL_NUMBER;channel_index++)
        {
          ppm[channel_index]=(msp_packet[5+channel_index*2+1]<<8)+msp_packet[5+channel_index*2];
         // Serial.print(channel_index);Serial.print(":");Serial.print(ppm[channel_index]);Serial.print(", ");
        }

        //Serial.println();
      }
      else
      {
        Serial.println("fi");
      }

      msp_packet_index=0;
    }
    else
    {
      msp_packet_index++;
    }
  }
  
  if((millis()-last_receive)>FAILSAFE)
  {
  	  Serial.print(millis());Serial.print(":");Serial.println(last_receive);
  	  
  	  //this will activate failsafe
  	  ppm[2]=FAILSAFE_THROTTLE_VALUE;
  }
  
  /*
    Here modify ppm array and set any channel to value between 1000 and 2000. 
    Timer running in the background will take care of the rest and automatically 
    generate PPM signal on output pin using values in ppm array
  */
  
}

ISR(TIMER1_COMPA_vect){  //leave this alone
  static boolean state = true;
  
  TCNT1 = 0;
  
  if (state) {  //start pulse
    digitalWrite(sigPin, onState);
    OCR1A = PULSE_LENGTH*2;
    state = false;
  } else{  //end pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;
  
    digitalWrite(sigPin, !onState);
    state = true;

    if(cur_chan_numb >= CHANNEL_NUMBER){
      cur_chan_numb = 0;
      calc_rest = calc_rest + PULSE_LENGTH;// 
      OCR1A = (FRAME_LENGTH - calc_rest)*2;
      calc_rest = 0;
    }
    else{
      OCR1A = (ppm[cur_chan_numb] - PULSE_LENGTH)*2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}
