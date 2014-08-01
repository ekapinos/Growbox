//Author: Alex R. Delp, April 19, 2014
//YL-38 Hygrometer Code

void setup()
{
  Serial.begin(9600);
  pinMode(A0, INPUT); //set up analog pin 0 to be input
}

void loop()
{
  int s = analogRead(A0); //take a sample
  
  //greater than 1000, probably not touching anything
  if(s >= 1000)
  { Serial.println("I think your prong's come loose."); }
  if(s < 1000 && s >= 650)
  //less than 1000, greater than 650, dry soil
  { Serial.println("Soil's rather dry, really."); }
  if(s < 650 && s >= 400)
  //less than 650, greater than 400, somewhat moist
  { Serial.println("Soil's a bit damp."); }
  if(s < 400)
  //less than 400, quite moist
  Serial.println("Soil is quite most, thank you very much.");
  
  delay(4000); //How often do you really need to take this reading?
}
