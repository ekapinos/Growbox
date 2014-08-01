//Author: Alex R. Delp, April 19, 2014
//YL-38 Hygrometer Code

void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT_PULLUP); //set up analog pin 0 to be input
  pinMode(22, OUTPUT); //set up analog pin 0 to be input
  
}

void loop(){
  digitalWrite(22, HIGH);

  
  for (int i = 0; i < 10; i++){
    int s = analogRead(A0); //take a sample
    Serial.print(" - ");Serial.print(s);
    delay(60);
  }
   delay(3000);
   int s = analogRead(A0); //take a sample
   Serial.print(" - ");Serial.print(s);
   
   Serial.println();
   
  digitalWrite(22, LOW);
  //boolean s1 = digitalRead(10); //take a sample
 // Serial.print("digital: ");Serial.println(s1);
//  //greater than 1000, probably not touching anything
//  if(s >= 1000)
//  { Serial.println("I think your prong's come loose."); }
//  if(s < 1000 && s >= 650)
//  //less than 1000, greater than 650, dry soil
//  { Serial.println("Soil's rather dry, really."); }
//  if(s < 650 && s >= 400)
//  //less than 650, greater than 400, somewhat moist
//  { Serial.println("Soil's a bit damp."); }
//  if(s < 400)
//  //less than 400, quite moist
//  Serial.println("Soil is quite most, thank you very much.");
  
  //digitalWrite(10, !digitalRead(10));
  //delay(15*60*1000); //How often do you really need to take this reading?
delay(1000);
}
